#include "nexora/router/router.hpp"
#include "nexora/router/node.hpp"
#include "nexora/utils.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

using namespace std;

namespace http {
    static auto segments(string_view path) {
        return path.substr(1) | views::split('/') | views::transform([](auto part) {
                   return string_view(part.begin(), part.end());
               });
    }

    static bool valid_pattern(string_view path) {
        if (!valid_path(path)) {
            return false;
        }

        bool wildcard_seen = false;
        for (auto key : segments(path)) {
            if (wildcard_seen || key == ":" || (key.starts_with('*') && key != "*")) {
                return false;
            }
            wildcard_seen = key == "*";
        }
        return true;
    }

    template <typename Node> struct PatternLookup {
        Node* node;
        vector<string_view> param_ids;
    };

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

    void Router::add(Method method, string_view path, function<void(Request&)> handler) {
        if (!valid_pattern(path)) {
            return;
        }

        vector<string> param_ids;
        Node* cur = &root_;
        for (auto key : segments(path)) {
            if (key[0] == '*') {
                cur = &cur->get_or_create_child("*", NodeType::Wildcard);
                break;
            }

            if (key[0] == ':') {
                cur = &cur->get_or_create_child(":", NodeType::Param);
                param_ids.emplace_back(key.substr(1));
                continue;
            }

            cur = &cur->get_or_create_child(string(key), NodeType::Path);
        }

        auto route = Route{
            .handler = std::move(handler),
            .param_ids = std::move(param_ids),
        };

        auto [it, inserted] = cur->routes.try_emplace(method, route);
        if (inserted) {
            ++size_;
        }
    }

    bool Router::contains(Method method, string_view path) const {
        auto [node, param_ids] = find_pattern(&root_, path);
        if (!node) {
            return false;
        }

        const Route* route = node->get_route(method);
        return route && ranges::equal(route->param_ids, param_ids);
    }

    void Router::remove(Method method, string_view path) {
        auto [node, param_ids] = find_pattern(&root_, path);
        if (!node || !node->try_remove_route(method, param_ids)) {
            return;
        }

        --size_;
        prune_empty_ancestors(node);
    }

    void Router::prune_empty_ancestors(Node* node) {
        while (node->parent && node->is_leaf() && node->routes.empty()) {
            auto* parent = node->parent;
            parent->children.erase(node->key);
            node = parent;
        }
    }

    optional<MatchResult> Router::match(Method method, string_view target) const {
        if (!valid_path(target)) {
            return nullopt;
        }

        vector<string_view> param_vals;
        const Node* node = match(&root_, method, target.substr(1), param_vals);
        if (!node) {
            return nullopt;
        }

        return MatchResult{node->get_route(method), std::move(param_vals)};
    }

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
            return node->routes.contains(method) ? node : nullptr;
        }

        size_t pos = target.find('/');
        string_view key = target.substr(0, pos);
        string_view remain = pos != string_view::npos ? target.substr(pos + 1) : "";

        const Node* child = get_or_default(node->children, string(key), nullptr);

        if (child && child->type == NodeType::Path) {
            if (const Node* matched = match(child, method, remain, param_vals)) {
                return matched;
            }
        }

        param_vals.push_back(key);

        const Node* matched = match(
            get_or_default(node->children, string(":"), nullptr),
            method,
            remain,
            param_vals
        );
        if (matched) {
            return matched;
        }

        param_vals.pop_back();

        const Node* wildcard = get_or_default(node->children, string("*"), nullptr);
        if (wildcard && wildcard->routes.contains(method)) {
            return wildcard;
        }

        return nullptr;
    }

    bool valid_path(std::string_view path) {
        return path.starts_with('/') && !path.contains("//") &&
               (path.size() == 1 || !path.ends_with('/'));
    }
} // namespace http
