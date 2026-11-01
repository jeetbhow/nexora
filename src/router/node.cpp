#include "nexora/router/node.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>

using namespace std;

namespace http {
    Node::Node(std::string_view key, NodeType type) : type(type), key(key) {
    }

    bool Node::is_leaf() {
        return children.empty();
    }

    const Node* Node::get_child(const std::string& key) const {
        auto it = children.find(key);
        return it != children.end() ? &it->second : nullptr;
    }

    Node* Node::get_child(const std::string& key) {
        auto it = children.find(key);
        return it != children.end() ? &it->second : nullptr;
    }

    Node& Node::get_or_create_child(const string& key, NodeType type) {
        string child_key = key;
        switch (type) {
            case NodeType::Path:
                break;
            case NodeType::Param:
                child_key = ":";
                break;
            case NodeType::Wildcard:
                child_key = "*";
                break;
        }

        auto [it, inserted] = children.try_emplace(child_key, child_key, type);
        if (inserted) {
            it->second.parent = this;
        }
        return it->second;
    }

    // Add a route handler if the node has none for its method yet. Param names
    // belong to each handler, so they play no part in the check. Return true if
    // the operation succeeded or false if it did not.
    bool Node::try_add_route_handler(Method method, Handler handler) {
        return handlers.try_emplace(method, std::move(handler)).second;
    }

    // Check if the node has a handler for the method, regardless of its param
    // ids.
    bool Node::has_route_handler(Method method) const {
        return handlers.contains(method);
    }

    const Handler* Node::get_route_handler(Method method) const {
        auto it = handlers.find(method);
        return it != handlers.end() ? &it->second : nullptr;
    }

    const Handler*
    Node::get_route_handler(Method method, const vector<string_view>& param_ids) const {
        auto* handler = get_route_handler(method);
        return handler && ranges::equal(handler->param_ids, param_ids) ? handler
                                                                       : nullptr;
    }

    bool
    Node::try_remove_route_handler(Method method, const vector<string_view>& param_ids) {
        auto it = handlers.find(method);
        if (it == handlers.end() || !ranges::equal(it->second.param_ids, param_ids)) {
            return false;
        }
        handlers.erase(it);
        return true;
    }
} // namespace http
