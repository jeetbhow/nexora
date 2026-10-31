#include "http/router.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>

using namespace std;

namespace http {
    namespace {
        // Views borrow the caller's path and are used only during the current operation.
        auto segments(string_view path) {
            return path.substr(1) |
                views::split('/') |
                views::transform([](auto part) { return string_view(part.begin(), part.end()); });
        }

        bool valid_pattern(string_view path) {
            if (!valid_path(path)) {
                return false;
            }
            for (auto key : segments(path)) {
                // Reject parameters with no name (a bare ':').
                // Reject wildcard segments until wildcard support is implemented.
                if (key == ":" || key.starts_with('*')) {
                    return false;
                }
            }
            return true;
        }

        template <typename Node>
        struct PatternLookup {
            Node* node;
            vector<string_view> param_ids;
        };

        // Preserve the root's constness so contains() and remove() share one walk.
        template <typename Node>
        PatternLookup<Node> find_pattern(Node* node, string_view path) {
            PatternLookup<Node> result{node, {}};
            if (!valid_pattern(path)) {
                result.node = nullptr;
                return result;
            }

            for (auto key : segments(path)) {
                if (key.starts_with(':')) {
                    result.param_ids.push_back(key.substr(1));
                    result.node = result.node->get_param_child();
                } else {
                    result.node = result.node->get_child(key);
                }

                if (!result.node) {
                    break;
                }
            }
            return result;
        }
    }

    RouterNode::RouterNode(std::string_view key) : key(key) {}

    // Get a child if it exists. Otherwise, return `nullptr`.
    RouterNode* RouterNode::get_child(std::string_view key) {
        return const_cast<RouterNode*>(as_const(*this).get_child(key));
    }

    const RouterNode* RouterNode::get_child(std::string_view key) const {
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

    const RouterNode* RouterNode::get_param_child() const {
        return param_child.get();
    }

    bool RouterNode::empty() const {
        return children.empty() && !param_child && handlers.empty();
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

    // Add a route handler if the node has none for its method yet. Param names belong to each
    // handler, so they play no part in the check. Return true if the operation succeeded or
    // false if it did not.
    bool RouterNode::try_add_route_handler(RouteHandler handler) {
        if (has_route_handler(handler.method)) {
            return false;
        }

        handlers.push_back(std::move(handler));
        return true;
    }

    // Check if the node has a handler for the method, regardless of its param ids.
    bool RouterNode::has_route_handler(Method method) const {
        return get_route_handler(method) != nullptr;
    }

    vector<RouteHandler>::const_iterator RouterNode::find_route_handler(Method method) const {
        return ranges::find(handlers, method, &RouteHandler::method);
    }

    const RouteHandler* RouterNode::get_route_handler(Method method) const {
        auto it = find_route_handler(method);
        return it != handlers.end() ? &*it : nullptr;
    }

    const RouteHandler* RouterNode::get_route_handler(
        Method method,
        const vector<string_view>& param_ids
    ) const {
        auto* handler = get_route_handler(method);
        return handler && ranges::equal(handler->param_ids, param_ids) ? handler : nullptr;
    }

    bool RouterNode::try_remove_route_handler(
        Method method,
        const vector<string_view>& param_ids
    ) {
        auto it = find_route_handler(method);
        if (it == handlers.end() || !ranges::equal(it->param_ids, param_ids)) {
            return false;
        }
        handlers.erase(it);
        return true;
    }

    void Router::add(Method method, string_view path) {
        if (!valid_pattern(path)) {
            return;
        }

        vector<string> param_ids;
        RouterNode* cur = &root_;
        for (auto key : segments(path)) {
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
            .param_ids = std::move(param_ids),
        };
        if (cur->try_add_route_handler(std::move(handler))) {
            ++size_;
        }
    }

    bool Router::contains(Method method, string_view path) const {
        auto [node, param_ids] = find_pattern(&root_, path);
        return node && node->get_route_handler(method, param_ids);
    }

    void Router::remove(Method method, string_view path) {
        auto [node, param_ids] = find_pattern(&root_, path);
        if (!node || !node->try_remove_route_handler(method, param_ids)) {
            return;
        }

        --size_;
        prune_empty_ancestors(node);
    }

    void Router::prune_empty_ancestors(RouterNode* node) {
        while (node->parent && node->empty()) {
            auto* parent = node->parent;
            if (parent->param_child.get() == node) {
                parent->param_child.reset();
            } else {
                parent->try_remove_child(node->key);
            }
            node = parent;
        }
    }

    bool Router::match(Method method, string_view target) const {
        if (!valid_path(target)) {
            return false;
        }
        return match(&root_, method, target.substr(1)) != nullptr;
    }

    // Recursively match the rest of `target` against the subtree rooted at `node`. Return the
    // node that ends the match, or `nullptr` if no route for `method` ends there. The static
    // child is tried first; the param child is tried whenever the static branch fails.
    const RouterNode* Router::match(const RouterNode* node, Method method, string_view target) {
        if (!node) {
            return nullptr;
        }

        if (target.empty()) {
            return node->has_route_handler(method) ? node : nullptr;
        }

        size_t pos = target.find('/');
        string_view key = target.substr(0, pos);
        string_view remain = pos != string_view::npos ? target.substr(pos + 1) : "";

        const RouterNode* matched = match(node->get_child(key), method, remain);
        return matched ? matched : match(node->get_param_child(), method, remain);
    }

    bool valid_path(std::string_view path) {
        return path.starts_with('/') &&
               !path.contains("//") &&
               (path.size() == 1 || !path.ends_with('/'));
    }
}
