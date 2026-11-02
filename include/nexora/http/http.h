#pragma once

#include <optional>
#include <string_view>

namespace nexora {
    /**
     * @brief HTTP request methods supported by the server.
     *
     * Requests using any other method are rejected with 501 Not Implemented.
     */
    enum class Method {
        Get,    ///< Retrieve a resource.
        Head,   ///< Like GET, but the response carries no body.
        Post,   ///< Submit data to a resource.
        Put,    ///< Replace a resource.
        Delete, ///< Remove a resource.
        Patch,  ///< Partially modify a resource.
        Options ///< Describe the communication options for a resource.
    };

    /**
     * @brief Returns the method's name as it appears in a request line.
     *
     * @param method The method to convert.
     * @return The uppercase method name, e.g. "GET".
     */
    std::string_view to_string(Method method);

    /**
     * @brief HTTP protocol versions supported by the server.
     *
     * Requests using any other version are rejected with 505 HTTP Version Not
     * Supported.
     */
    enum class Version {
        Http10, ///< HTTP/1.0
        Http11  ///< HTTP/1.1
    };

    /**
     * @brief HTTP status codes registered with IANA.
     *
     * Each enumerator's value is its numeric status code, so
     * `static_cast<int>(status)` gives the number to write in a status line.
     * Codes that IANA marks unused or reserved are left out.
     */
    enum class Status {
        // Informational
        Continue = 100,                      ///< The client should continue the request.
        SwitchingProtocols = 101,            ///< The server is switching protocols.
        Processing = 102,                    ///< The server is still working (WebDAV).
        EarlyHints = 103,                    ///< Preliminary headers.

        // Success
        Ok = 200,                            ///< The request succeeded.
        Created = 201,                       ///< A new resource was created.
        Accepted = 202,                      ///< Accepted for processing later.
        NonAuthoritativeInformation = 203,   ///< A proxy modified the origin's response.
        NoContent = 204,                     ///< Success with no body.
        ResetContent = 205,                  ///< The client should reset its view.
        PartialContent = 206,                ///< Only part of the resource was sent.
        MultiStatus = 207,                   ///< Several statuses (WebDAV).
        AlreadyReported = 208,               ///< Members were already listed (WebDAV).
        ImUsed = 226,                        ///< A delta encoding was applied.

        // Redirection
        MultipleChoices = 300,               ///< Several representations are available.
        MovedPermanently = 301,              ///< The resource has a new permanent URI.
        Found = 302,                         ///< The resource is temporarily at another URI.
        SeeOther = 303,                      ///< Get the response from another URI.
        NotModified = 304,                   ///< The cached copy is still valid.
        UseProxy = 305,                      ///< Deprecated: use the given proxy.
        TemporaryRedirect = 307,             ///< Repeat the request, same method, elsewhere.
        PermanentRedirect = 308,             ///< Repeat the request, same method, permanently.

        // Client errors
        BadRequest = 400,                    ///< Malformed request.
        Unauthorized = 401,                  ///< Authentication is required.
        PaymentRequired = 402,               ///< Reserved for future use.
        Forbidden = 403,                     ///< The server refuses to authorize.
        NotFound = 404,                      ///< No resource matches the target.
        MethodNotAllowed = 405,              ///< The resource doesn't allow the method.
        NotAcceptable = 406,                 ///< No representation fits `Accept`.
        ProxyAuthenticationRequired = 407,   ///< The proxy requires authentication.
        RequestTimeout = 408,                ///< The request took too long to arrive.
        Conflict = 409,                      ///< Conflicts with the resource's state.
        Gone = 410,                          ///< The resource is permanently gone.
        LengthRequired = 411,                ///< `Content-Length` is required.
        PreconditionFailed = 412,            ///< A request precondition was false.
        ContentTooLarge = 413,               ///< The body exceeds the server's limit.
        UriTooLong = 414,                    ///< The target exceeds the server's limit.
        UnsupportedMediaType = 415,          ///< The body's media type isn't supported.
        RangeNotSatisfiable = 416,           ///< The requested range can't be served.
        ExpectationFailed = 417,             ///< An `Expect` condition can't be met.
        ImATeapot = 418,                     ///< RFC 2324: the server is a teapot (joke status)
        MisdirectedRequest = 421,            ///< Sent to a server that can't answer it.
        UnprocessableContent = 422,          ///< Well-formed but semantically invalid.
        Locked = 423,                        ///< The resource is locked (WebDAV).
        FailedDependency = 424,              ///< A prior request failed (WebDAV).
        TooEarly = 425,                      ///< The server won't risk a replay.
        UpgradeRequired = 426,               ///< The client must switch protocols.
        PreconditionRequired = 428,          ///< The request must be conditional.
        TooManyRequests = 429,               ///< The client is rate limited.
        RequestHeaderFieldsTooLarge = 431,   ///< The header fields are too large.
        UnavailableForLegalReasons = 451,    ///< Blocked for legal reasons.

        // Server errors
        InternalServerError = 500,           ///< An unexpected server failure.
        NotImplemented = 501,                ///< The server doesn't support the method.
        BadGateway = 502,                    ///< An upstream server replied badly.
        ServiceUnavailable = 503,            ///< Overloaded or down for maintenance.
        GatewayTimeout = 504,                ///< An upstream server timed out.
        HttpVersionNotSupported = 505,       ///< The HTTP version isn't supported.
        VariantAlsoNegotiates = 506,         ///< Content negotiation is misconfigured.
        InsufficientStorage = 507,           ///< The server can't store the content.
        LoopDetected = 508,                  ///< The server found an infinite loop.
        NotExtended = 510,                   ///< Further extensions are required.
        NetworkAuthenticationRequired = 511, ///< The client must authenticate to the network.
    };

    /**
     * @brief Returns the status reason phrase, e.g. "OK" or "Not Found".
     *
     * The numeric status code is available through `static_cast<int>(status)`.
     */
    std::string_view to_string(Status status);

    std::optional<Method> parse_method(std::string_view s);
    std::optional<Version> parse_version(std::string_view s);
    std::string_view to_string(Version version);

    // Checks HTTP-version syntax, including versions the server doesn't support.
    bool is_http_version(std::string_view s);

    // Checks whether a character is allowed in an HTTP token.
    bool is_tchar(char c);

    /**
     * @brief Optional whitespace: the characters HTTP allows around field
     *        values and other tokens.
     *
     * OWS = *( SP / HTAB ) (RFC 9110 §5.6.3)
     */
    inline constexpr std::string_view ows = " \t";

    /**
     * @brief Compares two strings, ignoring ASCII case.
     *
     * Only the letters A-Z and a-z are folded, so non-ASCII bytes must match
     * exactly. Strings of different lengths are never equal. Used for header
     * field names, which are case-insensitive (RFC 9110 §5.1).
     *
     * @param a The first string, e.g. "Content-Type".
     * @param b The second string, e.g. "content-type".
     * @return `true` if the strings are equal apart from ASCII case.
     */
    bool case_insensitive_equals(std::string_view a, std::string_view b);

    /**
     * @brief Removes leading and trailing whitespace.
     *
     * Only spaces and tabs (`ows`) are removed. CR, LF and other whitespace
     * are kept, so a stray line ending is still visible to the caller.
     *
     * @param s The string to trim, e.g. "  text/html\t".
     * @return A view into `s` without the surrounding spaces and tabs, e.g.
     *         "text/html", or an empty view if `s` is all spaces and tabs.
     *         The view refers to the same characters as `s`, so it must not
     *         outlive them.
     */
    std::string_view trim(std::string_view s);

} // namespace nexora
