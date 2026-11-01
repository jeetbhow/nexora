#include "nexora/router/node.hpp"
#include "nexora/utils.hpp"

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
        return get_or_default(children, key, nullptr);
    }

    Node* Node::get_child(const std::string& key) {
        return get_or_default(children, key, nullptr);
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

    const Route* Node::get_route(Method method) const {
        return get_or_default(routes, method, nullptr);
    }

    bool Node::try_remove_route(Method method, const vector<string_view>& param_ids) {
        auto it = routes.find(method);
        if (it == routes.end() || !ranges::equal(it->second.param_ids, param_ids)) {
            return false;
        }
        routes.erase(it);
        return true;
    }
} // namespace http
