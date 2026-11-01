#pragma once

#include "nexora/http.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace http {
    struct Handler {
        std::function<void()> handler;
        std::vector<std::string> param_ids;
    };

    enum class NodeType { Path, Param, Wildcard };

    class Node {
    public:
        Node() = default;
        Node(std::string_view key, NodeType type);

        bool is_leaf();
        Node& get_or_create_child(const std::string& key, NodeType type);
        const Node* get_child(const std::string& key) const;
        Node* get_child(const std::string& key);

        // Route Handler Methods
        bool has_route_handler(Method method) const;
        const Handler* get_route_handler(Method method) const;
        const Handler* get_route_handler(
            Method method, const std::vector<std::string_view>& param_ids
        ) const;
        bool try_add_route_handler(Method method, Handler handler);
        bool try_remove_route_handler(
            Method method, const std::vector<std::string_view>& param_ids
        );

    private:
        friend class Router;

        NodeType type = NodeType::Path;
        std::string key = "";
        Node* parent = nullptr;
        std::unordered_map<std::string, Node> children{};
        std::unordered_map<Method, Handler> handlers;
    };
}; // namespace http
