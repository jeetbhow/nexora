#pragma once

#include "headers.hpp"
#include "http.h"
#include "nlohmann/json.hpp"

#include <string>

namespace nexora {
    struct Response {
        Version version = Version::Http11;
        Status status = Status::Ok;
        Headers headers;
        std::string body; 

        std::string serialize();
        void send(Status status, std::string body);
        void send(Status status, const char* body);
        void send(Status status, const nlohmann::json& body);
        void json(Status status, nlohmann::json body);
    };
} // namespace nexora
