#include "nexora/app.hpp"
#include "nexora/http/request.hpp"

#include <format>
#include <iostream>

using namespace std;
using namespace nexora::http;
using namespace nexora;

int main() {
    App app{};

    app.get("/echo/:param1/:param2", [](Request& req) {
        cout << req.to_string() << "\n";
    });

    app.get("/hello/:name", [](Request& req) {
        string name = req.params["name"];
        cout << format("Hello, {}!\n", name);
    });

    app.route(Method::Get, "/echo/p1/p2");
    app.route(Method::Get, "/hello/jeet");
}
