#include "nexora/http/response.hpp"

#include <format>
#include <print>
#include <string>

using namespace std;

namespace nexora {
    std::string Response::serialize() {
        string s;
        s += format("{} {} {}\r\n", to_string(version), static_cast<int>(status), to_string(status));
        for (auto& [key, val] : headers.fields) {
            s += format("{}: {}\r\n", key, val);
        }
        s += "\r\n";
        s += body;
        return s;
    }

    void Response::send(Status status, std::string body) {
        this->status = status;
        this->body = body;
        this->headers.set("Content-Type", "text/plain");
        this->headers.set("Content-Length", std::to_string(body.size()));
        string res_str = serialize();
        println("{}", res_str);
    }

    void Response::send(Status status, const char* body) {
        send(status, std::string(body));
    }

    void Response::send(Status status, const nlohmann::json& body) {
        this->status = status;
        this->body = body.dump();
        this->headers.set("Content-Type", "application/json");
        this->headers.set("Content-Length", std::to_string(this->body.size()));
        string res_str = serialize();
        println("{}", res_str);
    }
} // namespace nexora
