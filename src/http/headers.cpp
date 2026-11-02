#include "nexora/http/headers.hpp"
#include "nexora/http/http.h"

#include <cctype>
#include <functional>
#include <string>
#include <string_view>

using namespace std;

namespace nexora {
    size_t HeaderNameHash::operator()(string_view name) const {
        string lower(name);
        for (char& c : lower) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        return hash<string>{}(lower);
    }

    bool HeaderNameEqual::operator()(string_view a, string_view b) const {
        return case_insensitive_equals(a, b);
    }

    optional<string> Headers::get(string_view name) const {
        auto it = fields.find(name);
        if (it == fields.end()) {
            return nullopt;
        }

        return it->second;
    }

    bool Headers::add(string name, string value) {
        return fields.try_emplace(std::move(name), std::move(value)).second;
    }

    void Headers::set(string name, string value) {
        fields.insert_or_assign(std::move(name), std::move(value));
    }

    void Headers::remove(string_view name) {
        auto it = fields.find(name);
        if (it != fields.end()) {
            fields.erase(it);
        }
    }
} // namespace nexora
