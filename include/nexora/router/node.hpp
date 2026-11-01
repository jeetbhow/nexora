#pragma once

#include "nexora/http/http.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace http {
    struct Route {
        std::function<void(Request&)> handler;
        std::vector<std::string> param_ids;
    };

    struct MatchResult {
        const Route* route;
        std::vector<std::string_view> param_vals;
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

        const Route* get_route(Method method) const;
        bool try_remove_route(Method method, const std::vector<std::string_view>& param_ids);

    private:
        friend class Router;

        NodeType type = NodeType::Path;
        std::string key = "";
        Node* parent = nullptr;
        std::unordered_map<std::string, Node> children{};
        std::unordered_map<Method, Route> routes;
    };
}; // namespace http
