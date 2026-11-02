#pragma once

#include "http/request.hpp"
#include "http/response.hpp"
#include "router/router.hpp"

#include <functional>
#include <string_view>

namespace nexora {
    class App {
    public:
        void get(std::string_view path, std::function<void(Request&, Response&)> handler);
        void head(std::string_view path, std::function<void(Request&, Response&)> handler);
        void post(std::string_view path, std::function<void(Request&, Response&)> handler);
        void put(std::string_view path, std::function<void(Request&, Response&)> handler);
        // Named `del` because `delete` is a C++ keyword.
        void del(std::string_view path, std::function<void(Request&, Response&)> handler);
        void patch(std::string_view path, std::function<void(Request&, Response&)> handler);
        void options(std::string_view path, std::function<void(Request&, Response&)> handler);

        // Route a request to its matching handler and run it.
        void route(Method method, std::string_view target);

    private:
        Router router;
    };
} // namespace nexora
