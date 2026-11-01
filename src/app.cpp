#include "nexora/app.hpp"
#include "nexora/http/http.hpp"

#include <utility>

using namespace std;
using namespace http;

void App::get(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Get, path, std::move(handler));
}

void App::head(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Head, path, std::move(handler));
}

void App::post(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Post, path, std::move(handler));
}

void App::put(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Put, path, std::move(handler));
}

void App::del(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Delete, path, std::move(handler));
}

void App::patch(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Patch, path, std::move(handler));
}

void App::options(std::string_view path, std::function<void(http::Request&)> handler) {
    router.add(Method::Options, path, std::move(handler));
}

void App::route(http::Method method, std::string_view target) {
    optional<MatchResult> result = router.match(method, target);
    if (!result) {
        return;
    }

    auto req = Request{};
    for (size_t i = 0; i < result->param_vals.size(); i++) {
        const string& key = result->route->param_ids[i];
        const string& val = result->param_vals[i];
        req.params[std::move(key)] = std::move(val);
    }

    result->route->handler(req);
}
