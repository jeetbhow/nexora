#include "http/router/router.hpp"
#include "http/router/node.hpp"

#include <ranges>
#include <utility>

using namespace std;

namespace http {
    static auto segments(string_view path) {
        return path.substr(1) | views::split('/') |
               views::transform([](auto part) {
                   return string_view(part.begin(), part.end());
               });
    }

    static bool valid_pattern(string_view path) {
        if (!valid_path(path)) {
            return false;
        }

        for (auto key : segments(path)) {
            // Reject parameters with no name (a bare ':').
            // Reject wildcards until wildcard matching is enabled.
            if (key == ":" || key.starts_with('*')) {
                return false;
            }
        }
        return true;
    }

    template <typename Node> struct PatternLookup {
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
                result.node = result.node->get_child(":");
            } else {
                result.node = result.node->get_child(string(key));
            }

            if (!result.node) {
                break;
            }
        }
        return result;
    }

    void Router::add(Method method, string_view path) {
        if (!valid_pattern(path)) {
            return;
        }

        vector<string> param_ids;
        Node* cur = &root_;
        for (auto key : segments(path)) {
            if (key[0] == '*') {
                break;
            }

            if (key[0] == ':') {
                cur = &cur->get_or_create_child(":", NodeType::Param);
                param_ids.emplace_back(key.substr(1));
                continue;
            }

            cur = &cur->get_or_create_child(string(key), NodeType::Path);
        }

        auto handler = Handler{
            .handler = []() {}, // TODO: Add actual handlers.
            .param_ids = std::move(param_ids),
        };

        if (cur->try_add_route_handler(method, std::move(handler))) {
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

    void Router::prune_empty_ancestors(Node* node) {
        while (node->parent && node->is_leaf() && node->handlers.empty()) {
            auto* parent = node->parent;
            parent->children.erase(node->key);
            node = parent;
        }
    }

    bool Router::match(Method method, string_view target) const {
        if (!valid_path(target)) {
            return false;
        }

        vector<string_view> param_vals;
        return match(&root_, method, target.substr(1), param_vals) != nullptr;
    }

    // Recursively match the rest of `target` against the subtree rooted at
    // `node`. Return the node that ends the match, or `nullptr` if no route for
    // `method` ends there. Try static branches before parameter branches.
    const Node* Router::match(
        const Node* node,
        Method method,
        string_view target,
        vector<string_view>& param_vals
    ) {
        if (!node) {
            return nullptr;
        }

        if (target.empty()) {
            return node->has_route_handler(method) ? node : nullptr;
        }

        size_t pos = target.find('/');
        string_view key = target.substr(0, pos);
        string_view remain =
            pos != string_view::npos ? target.substr(pos + 1) : "";

        const Node* child = node->get_child(string(key));
        if (child && child->type == NodeType::Path) {
            if (const Node* matched =
                    match(child, method, remain, param_vals)) {
                return matched;
            }
        }

        param_vals.push_back(key);
        if (const Node* matched =
                match(node->get_child(":"), method, remain, param_vals)) {
            return matched;
        }

        param_vals.pop_back();

        return nullptr;
    }

    bool valid_path(std::string_view path) {
        return path.starts_with('/') && !path.contains("//") &&
               (path.size() == 1 || !path.ends_with('/'));
    }
} // namespace http
