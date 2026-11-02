#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nexora {
    /// Hashes a field name, ignoring ASCII case.
    struct HeaderNameHash {
        using is_transparent = void;
        size_t operator()(std::string_view name) const;
    };

    /// Compares field names, ignoring ASCII case.
    struct HeaderNameEqual {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const;
    };

    struct Headers {
        /**
         * @brief Header fields, keyed by name.
         *
         * Names are compared case-insensitively. There is one entry per name,
         * so duplicate fields are not allowed. Iteration order is unspecified.
         */
        std::unordered_map<std::string, std::string, HeaderNameHash, HeaderNameEqual> fields;

        /**
         * @brief Looks up the value of a header field.
         *
         * Names are compared case-insensitively.
         *
         * @param name The field name, e.g. "Content-Type".
         * @return The field's value, or `std::nullopt` if no field has that
         * name.
         */
        std::optional<std::string> get(std::string_view name) const;

        /**
         * @brief Looks up the value of a header field. Same as `get()`.
         *
         * Read-only: unlike `std::map::operator[]`, this never inserts a
         * field. Use `add()` or `set()` to insert one.
         *
         * @param name The field name, e.g. "Content-Type".
         * @return The field's value, or `std::nullopt` if no field has that
         * name.
         */
        std::optional<std::string> operator[](std::string_view name) const {
            return get(name);
        }

        /**
         * @brief Adds a header field.
         *
         * Duplicates are not allowed: if a field with the same name exists
         * (ignoring case), nothing changes. Use `set()` to replace a value.
         *
         * @param name  The field name.
         * @param value The field value.
         * @return `true` if the field was added, `false` if one with that
         *         name already exists.
         */
        bool add(std::string name, std::string value);

        /**
         * @brief Sets a header field, replacing any existing value.
         *
         * @param name  The field name.
         * @param value The field value.
         */
        void set(std::string name, std::string value);

        /**
         * @brief Removes the header field with the given name.
         *
         * Names are compared case-insensitively. Does nothing if no field has
         * that name.
         *
         * @param name The field name to remove.
         */
        void remove(std::string_view name);
    };
} // namespace nexora
