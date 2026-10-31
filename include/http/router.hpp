#pragma once

#include "http/http.hpp"

#include <functional>
#include <string>
#include <vector>
#include <string_view>
#include <memory>

// Test fixture that is allowed to inspect the router's trie.
class RouterTest;

namespace http {
    struct RouteHandler {
        Method method;
        std::function<void()> handler;
        std::vector<std::string> param_ids;
    };


    /**
     * @brief A single node in the router's trie.
     * 
     */
    class RouterNode {
    public:
        RouterNode() = default;
        RouterNode(std::string_view key);

        // Children Methods
        RouterNode* get_child(std::string_view key);
        const RouterNode* get_child(std::string_view key) const;
        RouterNode* get_or_create_child(std::string_view key);
        RouterNode* get_param_child();
        const RouterNode* get_param_child() const;
        RouterNode* get_or_create_param_child();
        bool try_remove_child(std::string_view key);
        bool empty() const;

        // Route Handler Methods
        bool has_route_handler(Method method) const;
        const RouteHandler* get_route_handler(Method method) const;
        const RouteHandler* get_route_handler(Method method, const std::vector<std::string_view>& param_ids) const;
        bool try_add_route_handler(RouteHandler handler);
        bool try_remove_route_handler(Method method, const std::vector<std::string_view>& param_ids);

    private:
        friend class Router;

        std::vector<RouteHandler>::const_iterator find_route_handler(Method method) const;

        RouterNode* parent = nullptr;
        std::vector<std::unique_ptr<RouterNode>> children{};
        std::unique_ptr<RouterNode> param_child = nullptr;
        std::vector<RouteHandler> handlers;
        std::string key = "";
    };

    /**
     * @brief Matches targets to routes. 
     * 
     */
    class Router {
    public:
        Router() = default;
        Router(Router&&) = delete;
        Router& operator=(Router&&) = delete;
        Router(const Router&) = delete;
        Router& operator=(const Router&) = delete;

        // Add a route pattern. Invalid patterns, including unsupported wildcards, are ignored.
        void add(Method method, std::string_view path);

        // Remove an exact registered pattern, including its parameter names.
        void remove(Method method, std::string_view path);

        // Check an exact registered pattern, including its parameter names.
        // Invalid patterns return false.
        bool contains(Method method, std::string_view path) const;

        // Match a concrete path, trying static branches before parameter branches.
        // Parameter names play no part in matching. Invalid paths return false.
        bool match(Method method, std::string_view target) const;

        // Return the number of routes.
        size_t size() const { return size_; }

    private:
        friend class ::RouterTest;

        static const RouterNode* match(const RouterNode* node, Method method, std::string_view target, std::vector<std::string_view>& param_vals);
        static void prune_empty_ancestors(RouterNode* node);

        /// The root of the trie. All potential routes branch off of this node.
        RouterNode root_{""};
        size_t size_ = 0;
    };

    // Validate the structure of a concrete path; ':' and '*' are ordinary target characters.
    bool valid_path(std::string_view path);
}
