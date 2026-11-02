#pragma once

#include "nexora/router/node.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

class RouterTest;

namespace nexora::router {
    class Router {
    public:
        Router() = default;
        Router(Router&&) = delete;
        Router& operator=(Router&&) = delete;
        Router(const Router&) = delete;
        Router& operator=(const Router&) = delete;

        void add(http::Method method, std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void remove(http::Method method, std::string_view path);
        bool contains(http::Method method, std::string_view path) const;
        std::optional<MatchResult> match(http::Method method, std::string_view target) const;

        size_t size() const { return size_; }

    private:
        friend class ::RouterTest;

        static const Node* match(
            const Node* node,
            http::Method method,
            std::string_view target,
            std::vector<std::string>& param_vals
        );

        static void prune_empty_ancestors(Node* node);

        Node root_{"", NodeType::Path};
        size_t size_ = 0;
    };

    bool valid_path(std::string_view path);
} // namespace nexora::router
