#pragma once

#include "headers.hpp"
#include "http.h"

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nexora::http {
    /**
     * @brief A parsed HTTP request.
     */
    struct Request {
        Request() = default;

        /// The header fields.
        Headers headers;

        std::unordered_map<std::string, std::string> params;

        /// The query parameters, e.g. {"sort": "asc"} for "?sort=asc".
        std::unordered_map<std::string, std::string> query_params;

        /// The request method, e.g. GET.
        Method method = Method::Get;

        /// The protocol version from the request line.
        Version version = Version::Http11;

        /// The request path without the query string, e.g. "/users/42".
        std::string path;

        /// The request body. Empty if the request has none.
        std::string body;

        /**
         * @brief Looks up a query parameter by name.
         *
         * Names are case-sensitive and compared after percent-decoding. If
         * the key appears more than once, the last value wins, e.g.
         * "?q=a&q=b" gives "b".
         *
         * @param name The parameter's decoded name, e.g. "sort".
         * @return The decoded value, or `std::nullopt` if there's no such
         *         parameter. A key with an empty value or no `=`, e.g. "?q="
         *         or "?q", gives "".
         */
        std::optional<std::string> query(const std::string& name) const;

        /**
         * @brief Formats the request as pretty-printed JSON, e.g. for logs and
         *        test output.
         *
         * Not HTTP wire format. Indented by two spaces, e.g.
         *
         *     {
         *       "method": "GET",
         *       "path": "/users/42",
         *       "version": "HTTP/1.1",
         *       "query": {
         *         "sort": "asc"
         *       },
         *       "headers": {
         *         "Host": "example.com"
         *       },
         *       "body": "hello"
         *     }
         *
         * `headers` is an object keyed by field name; its order is
         * unspecified. Strings are escaped with
         * `json_quote`, so the output is only valid JSON if the request's
         * strings are valid UTF-8.
         *
         * @return The request as a JSON object.
         */
        std::string to_string() const;
    };

    /**
     * @brief Reasons a request can fail to parse.
     */
    enum class ParseError {
        BadRequest,        ///< Malformed request. Respond with 400 Bad Request.
        UnsupportedMethod, ///< Method isn't one of `Method`. Respond with 501
        UnsupportedVersion ///< Version isn't one of `Version`. Respond with 505
    };

    /**
     * @brief Parses a single HTTP request.
     *
     * Fails with:
     * - `ParseError::BadRequest` if the request line or a header line is
     *   malformed, a header field appears more than once, a query parameter has a bad `%XX` escape,
     *   `Content-Length` is invalid, or the body is shorter than
     *   `Content-Length` says;
     * - `ParseError::UnsupportedMethod` if the method isn't one of `Method`;
     * - `ParseError::UnsupportedVersion` if the version isn't one of `Version`.
     *
     * @param request The raw bytes of one complete request: request line,
     *                header fields, and body.
     *
     * @return The parsed request, or the reason it couldn't be parsed.
     */
    std::expected<Request, ParseError> parse_request(std::string_view request);
} // namespace nexora::http
