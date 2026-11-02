#include "nexora/app.hpp"
#include "nexora/http/request.hpp"

#include <utility>

using namespace std;

namespace nexora {
    using Handler = std::function<void(Request&, Response&)>;

    void App::get(std::string_view path, Handler handler) {
        router.add(Method::Get, path, std::move(handler));
    }

    void App::head(std::string_view path, Handler handler) {
        router.add(Method::Head, path, std::move(handler));
    }

    void App::post(std::string_view path, Handler handler) {
        router.add(Method::Post, path, std::move(handler));
    }

    void App::put(std::string_view path, Handler handler) {
        router.add(Method::Put, path, std::move(handler));
    }

    void App::del(std::string_view path, Handler handler) {
        router.add(Method::Delete, path, std::move(handler));
    }

    void App::patch(std::string_view path, Handler handler) {
        router.add(Method::Patch, path, std::move(handler));
    }

    void App::options(std::string_view path, Handler handler) {
        router.add(Method::Options, path, std::move(handler));
    }

    void App::route(Method method, std::string_view target) {
        optional<MatchResult> result = router.match(method, target);
        if (!result) {
            return;
        }

        auto req = Request{};
        req.method = method;
        auto res = Response{};
        res.version = req.version;
        res.status = Status::Ok;
        for (size_t i = 0; i < result->param_vals.size(); i++) {
            const string& key = result->route->param_ids[i];
            const string& val = result->param_vals[i];
            req.params[std::move(key)] = std::move(val);
        }

        result->route->handler(req, res);
    }

} // namespace nexora
