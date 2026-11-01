#include "nexora/app.hpp"
#include "nexora/http/http.hpp"

#include <iostream>

using namespace std;
using namespace http;

int main() {
    App app{};

    app.get("/echo", [](Request& req) { cout << req.to_string() << "\n"; });

    app.get("/hello", [](Request&) { cout << "Hello, world!\n"; });

    app.route(Method::Get, "/hello");
}
