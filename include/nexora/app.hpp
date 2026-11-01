#pragma once

#include "http/http.hpp"
#include "router/router.hpp"

#include <functional>
#include <string_view>

class App {
public:
    void get(std::string_view path, std::function<void(http::Request)> handler);
    void post(std::string_view path, std::function<void(http::Request)> handler);
    void put(std::string_view path, std::function<void(http::Request)> handler);

private:
    http::Router router;
};