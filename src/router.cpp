#include "http/router.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

using namespace std;

namespace http {
    namespace {
        bool valid_pattern(string_view path) {
            if (!valid_path(path)) {
                return false;
            }
            for (auto part : path.substr(1) | views::split('/')) {
                auto key = string_view(part.begin(), part.end());
                // Reject parameters with no name (a bare ':').
                // Reject wildcard segments until wildcard support is implemented.
                if (key == ":" || key.starts_with('*')) {
                    return false;
                }
            }
            return true;
        }
    }

    RouterNode::RouterNode(std::string_view key) : key(key) {}

    // Get a child if it exists. Otherwise, return `nullptr`.
    RouterNode* RouterNode::get_child(std::string_view key) {
        auto it = ranges::find(children, key, &RouterNode::key);
        return it != children.end() ? it->get() : nullptr;
    }

    // Get a child if it exists, otherwise create one, set its parent to this node and return it.
    RouterNode* RouterNode::get_or_create_child(std::string_view key) {
        if (auto* child = get_child(key)) {
            return child;
        }

        auto child = make_unique<RouterNode>(key);
        child->parent = this;
        return children.emplace_back(std::move(child)).get();
    }

    bool RouterNode::try_remove_child(std::string_view key) {
        auto it = ranges::find(children, key, &RouterNode::key);
        if (it != children.end()) {
            children.erase(it);
            return true;
        } else {
            return false;
        }
    }

    // Get the param child if it exists. Otherwise, return `nullptr`.
    RouterNode* RouterNode::get_param_child() {
        return param_child.get();
    }

    // Get the param child if it exists, otherwise create it, set its parent to this node and return it.
    RouterNode* RouterNode::get_or_create_param_child() {
        if (param_child) {
            return param_child.get();
        }

        param_child = make_unique<RouterNode>();
        param_child->parent = this;
        return param_child.get();
    }

    void RouterNode::create_child(string_view key) {
        get_or_create_child(key);
    }

    bool RouterNode::try_add_route_handler(const RouteHandler& handler) {
        if (has_route_handler(handler.method)) {
            return false;
        }
        handlers.push_back(handler);
        return true;
    }

    bool RouterNode::has_route_handler(Method method) const {
        return ranges::contains(handlers, method, &RouteHandler::method);
    }

    bool RouterNode::has_route_handler(Method method, const vector<string_view>& param_ids) {
        return get_route_handler(method, param_ids) != nullptr;
    }

    const RouteHandler* RouterNode::get_route_handler(
        Method method,
        const vector<string_view>& param_ids
    ) const {
        auto it = ranges::find_if(handlers, [&](const RouteHandler& handler) {
            return handler.method == method && ranges::equal(handler.param_ids, param_ids);
        });
        return it != handlers.end() ? &*it : nullptr;
    }

    bool RouterNode::try_remove_route_handler(
        Method method,
        const vector<string_view>& param_ids
    ) {
        auto* handler = get_route_handler(method, param_ids);
        if (!handler) {
            return false;
        }
        handlers.erase(handlers.begin() + (handler - handlers.data()));
        return true;
    }

    void Router::add(Method method, string_view path) {
        if (!valid_pattern(path)) {
            return;
        }

        vector<string> param_ids;
        RouterNode* cur = &root_;
        for (auto part : path.substr(1) | views::split('/')) {
            auto key = string_view(part.begin(), part.end());
            if (key.starts_with(':')) {
                cur = cur->get_or_create_param_child();
                param_ids.emplace_back(key.substr(1));
            } else {
                cur = cur->get_or_create_child(key);
            }
        }

        auto handler = RouteHandler {
            .method = method,
            .handler = []() {},             // TODO: Add actual handlers.
            .param_ids = param_ids,
        };
        if (cur->try_add_route_handler(handler)) {
            ++size_;
        }
    }

    bool Router::contains(Method method, string_view path) {
        if (!valid_pattern(path)) {
            return false;
        }

        vector<string_view> param_ids;
        RouterNode* node = &root_;
        for (auto part : path.substr(1) | views::split('/')) {
            auto key = string_view(part.begin(), part.end());
            if (key.starts_with(':')) {
                param_ids.push_back(key.substr(1));
                node = node->get_param_child();
            } else {
                node = node->get_child(key);
            }
            if (!node) {
                return false;
            }
        }
        return node->get_route_handler(method, param_ids) != nullptr;
    }

    void Router::remove(Method method, string_view path) {
        if (!valid_pattern(path)) {
            return;
        }

        vector<string_view> param_ids;
        RouterNode* node = &root_;
        for (auto part : path.substr(1) | views::split('/')) {
            auto key = string_view(part.begin(), part.end());
            if (key.starts_with(':')) {
                param_ids.push_back(key.substr(1));
                node = node->get_param_child();
            } else {
                node = node->get_child(key);
            }
            if (!node) {
                return;
            }
        }
        if (!node->try_remove_route_handler(method, param_ids)) {
            return;
        }
        --size_;

        while (node->parent && node->children.empty() &&
               !node->param_child && node->handlers.empty()) {
            auto* parent = node->parent;
            if (parent->param_child.get() == node) {
                parent->param_child.reset();
            } else {
                parent->try_remove_child(node->key);
            }
            node = parent;
        }
    }

    bool Router::match(Method method, string_view target) {
        if (!valid_path(target)) {
            return false;
        }
        return match(&root_, method, target.substr(1)) != nullptr;
    }

    // Recursively match the rest of `target` against the subtree rooted at `node`. Return the
    // node that ends the match, or `nullptr` if no route for `method` ends there. The static
    // child is tried first; the param child is tried whenever the static branch fails.
    RouterNode* Router::match(RouterNode* node, Method method, string_view target) {
        if (!node) {
            return nullptr;
        }

        if (target.empty()) {
            return node->has_route_handler(method) ? node : nullptr;
        }

        size_t pos = target.find('/');
        string_view key = target.substr(0, pos);
        string_view remain = pos != string_view::npos ? target.substr(pos + 1) : "";

        RouterNode* matched = match(node->get_child(key), method, remain);
        return matched ? matched : match(node->get_param_child(), method, remain);
    }

    bool valid_path(std::string_view path) {
        return path.starts_with('/') &&
               !path.contains("//") &&
               (path.size() == 1 || !path.ends_with('/'));
    }
}
