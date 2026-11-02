#include "nexora/http/http.h"

#include <algorithm>
#include <cctype>
#include <utility>

using namespace std;

namespace nexora {
    optional<Method> parse_method(string_view s) {
        // Methods are case-sensitive (RFC 9110 §9.1), so "get" is not GET.
        if (s == "GET")
            return Method::Get;
        if (s == "HEAD")
            return Method::Head;
        if (s == "POST")
            return Method::Post;
        if (s == "PUT")
            return Method::Put;
        if (s == "DELETE")
            return Method::Delete;
        if (s == "PATCH")
            return Method::Patch;
        if (s == "OPTIONS")
            return Method::Options;
        return nullopt;
    }

    string_view to_string(Method method) {
        switch (method) {
            case Method::Get:
                return "GET";
            case Method::Head:
                return "HEAD";
            case Method::Post:
                return "POST";
            case Method::Put:
                return "PUT";
            case Method::Delete:
                return "DELETE";
            case Method::Patch:
                return "PATCH";
            case Method::Options:
                return "OPTIONS";
        }
        unreachable();
    }

    string_view to_string(Version version) {
        switch (version) {
            case Version::Http10:
                return "HTTP/1.0";
            case Version::Http11:
                return "HTTP/1.1";
        }
        unreachable();
    }

    string_view to_string(Status status) {
        switch (status) {
            case Status::Continue:
                return "Continue";
            case Status::SwitchingProtocols:
                return "Switching Protocols";
            case Status::Processing:
                return "Processing";
            case Status::EarlyHints:
                return "Early Hints";
            case Status::Ok:
                return "OK";
            case Status::Created:
                return "Created";
            case Status::Accepted:
                return "Accepted";
            case Status::NonAuthoritativeInformation:
                return "Non-Authoritative Information";
            case Status::NoContent:
                return "No Content";
            case Status::ResetContent:
                return "Reset Content";
            case Status::PartialContent:
                return "Partial Content";
            case Status::MultiStatus:
                return "Multi-Status";
            case Status::AlreadyReported:
                return "Already Reported";
            case Status::ImUsed:
                return "IM Used";
            case Status::MultipleChoices:
                return "Multiple Choices";
            case Status::MovedPermanently:
                return "Moved Permanently";
            case Status::Found:
                return "Found";
            case Status::SeeOther:
                return "See Other";
            case Status::NotModified:
                return "Not Modified";
            case Status::UseProxy:
                return "Use Proxy";
            case Status::TemporaryRedirect:
                return "Temporary Redirect";
            case Status::PermanentRedirect:
                return "Permanent Redirect";
            case Status::BadRequest:
                return "Bad Request";
            case Status::Unauthorized:
                return "Unauthorized";
            case Status::PaymentRequired:
                return "Payment Required";
            case Status::Forbidden:
                return "Forbidden";
            case Status::NotFound:
                return "Not Found";
            case Status::MethodNotAllowed:
                return "Method Not Allowed";
            case Status::NotAcceptable:
                return "Not Acceptable";
            case Status::ProxyAuthenticationRequired:
                return "Proxy Authentication Required";
            case Status::RequestTimeout:
                return "Request Timeout";
            case Status::Conflict:
                return "Conflict";
            case Status::Gone:
                return "Gone";
            case Status::LengthRequired:
                return "Length Required";
            case Status::PreconditionFailed:
                return "Precondition Failed";
            case Status::ContentTooLarge:
                return "Content Too Large";
            case Status::UriTooLong:
                return "URI Too Long";
            case Status::UnsupportedMediaType:
                return "Unsupported Media Type";
            case Status::RangeNotSatisfiable:
                return "Range Not Satisfiable";
            case Status::ExpectationFailed:
                return "Expectation Failed";
            case Status::ImATeapot:
                return "I'm a teapot";
            case Status::MisdirectedRequest:
                return "Misdirected Request";
            case Status::UnprocessableContent:
                return "Unprocessable Content";
            case Status::Locked:
                return "Locked";
            case Status::FailedDependency:
                return "Failed Dependency";
            case Status::TooEarly:
                return "Too Early";
            case Status::UpgradeRequired:
                return "Upgrade Required";
            case Status::PreconditionRequired:
                return "Precondition Required";
            case Status::TooManyRequests:
                return "Too Many Requests";
            case Status::RequestHeaderFieldsTooLarge:
                return "Request Header Fields Too Large";
            case Status::UnavailableForLegalReasons:
                return "Unavailable For Legal Reasons";
            case Status::InternalServerError:
                return "Internal Server Error";
            case Status::NotImplemented:
                return "Not Implemented";
            case Status::BadGateway:
                return "Bad Gateway";
            case Status::ServiceUnavailable:
                return "Service Unavailable";
            case Status::GatewayTimeout:
                return "Gateway Timeout";
            case Status::HttpVersionNotSupported:
                return "HTTP Version Not Supported";
            case Status::VariantAlsoNegotiates:
                return "Variant Also Negotiates";
            case Status::InsufficientStorage:
                return "Insufficient Storage";
            case Status::LoopDetected:
                return "Loop Detected";
            case Status::NotExtended:
                return "Not Extended";
            case Status::NetworkAuthenticationRequired:
                return "Network Authentication Required";
        }
        unreachable();
    }

    // HTTP-version = "HTTP/" DIGIT "." DIGIT
    bool is_http_version(string_view s) {
        auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
        return s.size() == 8 && s.starts_with("HTTP/") && is_digit(s[5]) && s[6] == '.' &&
               is_digit(s[7]);
    }

    optional<Version> parse_version(string_view s) {
        if (s == "HTTP/1.0")
            return Version::Http10;
        if (s == "HTTP/1.1")
            return Version::Http11;
        return nullopt;
    }

    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
    //         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA (RFC 9110 §5.6.2)
    bool is_tchar(char c) {
        return isalnum(static_cast<unsigned char>(c)) || "!#$%&'*+-.^_`|~"sv.contains(c);
    }

    bool case_insensitive_equals(string_view a, string_view b) {
        return ranges::equal(a, b, [](char x, char y) {
            return tolower(static_cast<unsigned char>(x)) ==
                   tolower(static_cast<unsigned char>(y));
        });
    }

    string_view trim(string_view s) {
        size_t first = s.find_first_not_of(ows);
        if (first == string_view::npos) {
            return "";
        }

        size_t last = s.find_last_not_of(ows);
        return s.substr(first, last - first + 1);
    }

} // namespace nexora
