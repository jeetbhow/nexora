#pragma once

#include "http/request.hpp"
#include "http/response.hpp"
#include "router/router.hpp"

#include <functional>
#include <string_view>

namespace nexora {
    class App {
    public:
        void get(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void head(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void post(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void put(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        // Named `del` because `delete` is a C++ keyword.
        void del(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void patch(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);
        void options(std::string_view path, std::function<void(http::Request&, http::Response&)> handler);

        // Route a request to its matching handler and run it.
        void route(http::Method method, std::string_view target);

    private:
        router::Router router;
    };
} // namespace nexora
