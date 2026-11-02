#include "nexora/app.hpp"
#include "nexora/http/http.h"
#include "nexora/http/request.hpp"
#include "nexora/http/response.hpp"

#include <format>
#include <print>

using namespace std;
using namespace nexora;

int main() {
    App app{};
    app.get("/hello/:name", [](Request& req, Response& res) {
        string name = req.params["name"];

        nlohmann::json body = {
            {"message", "Hello"},
            {"status", 200},
        };

        res.send(Status::Ok, body);
    });

    app.route(Method::Get, "/hello/jeet");
}
