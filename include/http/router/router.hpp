#pragma once

#include "http/router/node.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

// Test fixture that is allowed to inspect the router's trie.
class RouterTest;

namespace http {
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

        // Add a route pattern. A terminal '*' matches one or more remaining
        // segments. Invalid patterns are ignored.
        void add(Method method, std::string_view path);

        // Remove an exact registered pattern, including its parameter names.
        void remove(Method method, std::string_view path);

        // Check an exact registered pattern, including its parameter names.
        // Invalid patterns return false.
        bool contains(Method method, std::string_view path) const;

        // Match a concrete path, trying static, parameter, then wildcard
        // branches. Parameter names play no part in matching. Invalid paths
        // return false. Unnamed wildcards do not add parameter captures.
        bool match(Method method, std::string_view target) const;

        // Return the number of routes.
        size_t size() const { return size_; }

    private:
        friend class ::RouterTest;

        static const Node* match(
            const Node* node,
            Method method,
            std::string_view target,
            std::vector<std::string_view>& param_vals
        );

        static void prune_empty_ancestors(Node* node);

        /// The root of the trie. All potential routes branch off of this node.
        Node root_{"", NodeType::Path};
        size_t size_ = 0;
    };

    // Validate the structure of a concrete path; ':' and '*' are ordinary
    // target characters.
    bool valid_path(std::string_view path);
} // namespace http
