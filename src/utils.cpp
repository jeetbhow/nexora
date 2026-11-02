#include "nexora/utils.hpp"

using namespace std;

namespace nexora::http {
    optional<pair<string_view, string_view>> split_once(string_view s, char delim) {
        size_t pos = s.find(delim);
        if (pos == string_view::npos) {
            return nullopt;
        }

        return pair{s.substr(0, pos), s.substr(pos + 1)};
    }

    string json_quote(string_view s) {
        string result = "\"";

        for (char c : s) {
            switch (c) {
                case '"':
                    result += "\\\"";
                    break;
                case '\\':
                    result += "\\\\";
                    break;
                case '\b':
                    result += "\\b";
                    break;
                case '\f':
                    result += "\\f";
                    break;
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                case '\t':
                    result += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        constexpr string_view hex = "0123456789abcdef";
                        result += "\\u00";
                        result += hex[(c >> 4) & 0xF];
                        result += hex[c & 0xF];
                    } else {
                        result += c;
                    }
            }
        }

        result += '"';
        return result;
    }

    string to_string(const unordered_map<string, string>& map) {
        if (map.empty()) {
            return "{}";
        }

        string result = "{";
        bool first = true;

        for (const auto& [key, value] : map) {
            result += first ? "\n  " : ",\n  ";
            first = false;

            result += json_quote(key);
            result += ": ";
            result += json_quote(value);
        }

        result += "\n}";
        return result;
    }

    string to_string(const vector<pair<string, string>>& pairs) {
        if (pairs.empty()) {
            return "[]";
        }

        string result = "[";
        bool first = true;

        for (const auto& [name, value] : pairs) {
            result += first ? "\n  " : ",\n  ";
            first = false;

            result += '[' + json_quote(name) + ", " + json_quote(value) + ']';
        }

        result += "\n]";
        return result;
    }
} // namespace nexora::http
