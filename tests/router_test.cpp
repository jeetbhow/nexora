#include <gtest/gtest.h>

#include "nexora/router/router.hpp"

#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace nexora;

const auto noop = [](Request&, Response&) {};

class RouterTest : public testing::Test {
protected:
    // Walk static branches, stopping at the last existing node.
    Node* find(std::string_view path) {
        auto* node = &router.root_;
        for (auto part : path.substr(1) | std::views::split('/')) {
            auto* child = node->get_child(std::string(part.begin(), part.end()));
            if (!child) {
                break;
            }
            node = child;
        }
        return node;
    }

    // Inspect captures from the recursive matcher until the public API exposes
    // them.
    const Node* match_with_params(
        Method method, std::string_view target, std::vector<std::string>& param_vals
    ) {
        return Router::match(&router.root_, method, target.substr(1), param_vals);
    }

    Router router;
};

// Router::add()

TEST_F(RouterTest, AddInvalidPathIsNoOp) {
    router.add(Method::Get, "/a", noop);

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.add(Method::Get, path, noop);

        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, AddRejectsParamSegmentWithDifferentNameAtSamePosition) {
    router.add(Method::Get, "/a/:c", noop);
    router.add(Method::Get, "/a/:b", noop);

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_FALSE(router.contains(Method::Get, "/a/:b"));
}

TEST_F(RouterTest, AddAllowsDifferentParamNameWithDifferentMethod) {
    router.add(Method::Get, "/a/:c", noop);
    router.add(Method::Post, "/a/:b", noop);

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_TRUE(router.contains(Method::Post, "/a/:b"));

    // Each handler keeps its own param names.
    EXPECT_FALSE(router.contains(Method::Get, "/a/:b"));
    EXPECT_FALSE(router.contains(Method::Post, "/a/:c"));
}

TEST_F(RouterTest, AddAllowsDifferentParamNamesInSharedPrefix) {
    router.add(Method::Get, "/a/:b/x", noop);
    router.add(Method::Get, "/a/:c/y", noop);

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:b/x"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c/y"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1/x"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1/y"));
}

TEST_F(RouterTest, AddRejectsConflictingParamNameAtRoot) {
    router.add(Method::Get, "/:a", noop);
    router.add(Method::Get, "/:b", noop);

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/:b"));
}

TEST_F(RouterTest, AddAllowsSameParamNameWithDifferentMethod) {
    router.add(Method::Get, "/a/:c", noop);
    router.add(Method::Post, "/a/:c", noop);

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_TRUE(router.contains(Method::Post, "/a/:c"));
}

TEST_F(RouterTest, AddRejectsParamSegmentWithEmptyName) {
    router.add(Method::Get, "/:", noop);

    EXPECT_EQ(router.size(), 0u);
    EXPECT_FALSE(router.contains(Method::Get, "/:"));
}

TEST_F(RouterTest, InvalidWildcardPatternsAreRejectedWithoutChangingExistingRoutes) {
    router.add(Method::Get, "/", noop);
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a/b", noop);

    for (const auto pattern : {"/a/*rest", "/a/*/b", "/new/*/b", "/a/*/*"}) {
        SCOPED_TRACE(pattern);
        // Post would create a distinct handler if the wildcard were silently
        // skipped.
        router.add(Method::Post, pattern, noop);
        EXPECT_EQ(router.size(), 3u);
        EXPECT_FALSE(router.contains(Method::Post, pattern));
        EXPECT_FALSE(router.match(Method::Post, "/"));
        EXPECT_FALSE(router.match(Method::Post, "/a"));
        EXPECT_FALSE(router.match(Method::Post, "/a/b"));
        EXPECT_FALSE(router.contains(Method::Get, pattern));

        router.remove(Method::Get, pattern);
        EXPECT_EQ(router.size(), 3u);
        EXPECT_TRUE(router.contains(Method::Get, "/"));
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
        EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
    }
    EXPECT_EQ(find("/")->get_child("new"), nullptr);
}

TEST_F(RouterTest, WildcardRegistersAndRemovesAnExactPattern) {
    router.add(Method::Get, "/a/*", noop);
    router.add(Method::Get, "/a/*", noop);
    router.add(Method::Post, "/a/*", noop);
    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/*"));
    EXPECT_FALSE(router.contains(Method::Get, "/a"));
    EXPECT_FALSE(router.contains(Method::Get, "/a/b"));

    router.remove(Method::Get, "/a");
    EXPECT_EQ(router.size(), 2u);
    router.remove(Method::Get, "/a/*");
    EXPECT_FALSE(router.match(Method::Get, "/a/b"));
    EXPECT_TRUE(router.match(Method::Post, "/a/b/c"));
    router.remove(Method::Post, "/a/*");
    EXPECT_EQ(router.size(), 0u);
    EXPECT_EQ(find("/")->get_child("a"), nullptr);
}

TEST_F(RouterTest, WildcardMatchesOneOrMoreRemainingSegments) {
    router.add(Method::Get, "/a/*", noop);
    for (const auto target : {"/a/b", "/a/b/c", "/a/*", "/a/:"}) {
        SCOPED_TRACE(target);
        EXPECT_TRUE(router.match(Method::Get, target));
    }
    EXPECT_FALSE(router.match(Method::Get, "/a"));
    EXPECT_FALSE(router.match(Method::Get, "/a/"));
    EXPECT_FALSE(router.match(Method::Get, "/ab/c"));
    EXPECT_FALSE(router.match(Method::Post, "/a/b"));
    router.add(Method::Get, "/a", noop);
    EXPECT_TRUE(router.match(Method::Get, "/a"));
    router.remove(Method::Get, "/a/*");
    EXPECT_TRUE(router.match(Method::Get, "/a"));
}

TEST_F(RouterTest, RootWildcardDoesNotMatchRoot) {
    router.add(Method::Get, "/*", noop);
    EXPECT_FALSE(router.match(Method::Get, "/"));
    EXPECT_TRUE(router.match(Method::Get, "/a"));
    EXPECT_TRUE(router.match(Method::Get, "/a/b"));
}

TEST_F(RouterTest, WildcardIsTriedAfterStaticAndParameterBranches) {
    router.add(Method::Get, "/a/*", noop);
    router.add(Method::Get, "/a/:id", noop);
    router.add(Method::Get, "/a/fixed", noop);

    std::vector<std::string> values;
    EXPECT_EQ(match_with_params(Method::Get, "/a/fixed", values), find("/a/fixed"));
    EXPECT_TRUE(values.empty());
    EXPECT_EQ(match_with_params(Method::Get, "/a/other", values), find("/a/:"));
    EXPECT_EQ(values, (std::vector<std::string>{"other"}));
    values.clear();
    EXPECT_EQ(match_with_params(Method::Get, "/a/other/tail", values), find("/a/*"));
    EXPECT_TRUE(values.empty());
}

TEST_F(RouterTest, WildcardPreservesEarlierNamedCaptures) {
    router.add(Method::Get, "/:id/*", noop);
    std::vector<std::string> values;
    ASSERT_NE(match_with_params(Method::Get, "/42/b/c", values), nullptr);
    EXPECT_EQ(values, (std::vector<std::string>{"42"}));
    EXPECT_FALSE(router.match(Method::Get, "/42"));
}

TEST_F(RouterTest, AddRegistersSingleRoute) {
    router.add(Method::Get, "/a", noop);

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, AddRegistersTwoRoutesWithDifferentMethods) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);
    router.add(second, "/b", noop);

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_b = router.contains(second, "/b");
    EXPECT_TRUE(second_b);

    auto first_b = router.contains(first, "/b");
    EXPECT_FALSE(first_b);

    auto second_a = router.contains(second, "/a");
    EXPECT_FALSE(second_a);
}

// Router::contains()

TEST_F(RouterTest, ContainsInvalidPathReturnsFalse) {
    router.add(Method::Get, "/a", noop);

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.contains(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, ContainsReturnsFalseForUnaddedRoute) {
    router.add(Method::Get, "/a", noop);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_FALSE(b);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);
}

TEST_F(RouterTest, ContainsReturnsFalseForIntermediateNode) {
    router.add(Method::Get, "/a/b", noop);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);
}

TEST_F(RouterTest, ContainsParamRouteOnlyForRegisteredPattern) {
    router.add(Method::Get, "/:x", noop);

    EXPECT_TRUE(router.contains(Method::Get, "/:x"));
    EXPECT_FALSE(router.contains(Method::Get, "/a"));
}

TEST_F(RouterTest, ContainsReturnsFalseForDifferentMethod) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);

    auto a = router.contains(second, "/a");
    EXPECT_FALSE(a);
}

// Router::match()

TEST_F(RouterTest, MatchInvalidPathReturnsFalse) {
    router.add(Method::Get, "/a", noop);

    for (const auto path : {"", "a", "//", "/a/"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.match(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, MatchRegisteredPathsMatch) {
    router.add(Method::Get, "/", noop);
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a/b", noop);

    for (const auto target : {"/", "/a", "/a/b"}) {
        SCOPED_TRACE(target);
        EXPECT_TRUE(router.match(Method::Get, target).has_value());
    }
}

TEST_F(RouterTest, MatchIntermediateNodeWithoutRouteRejects) {
    router.add(Method::Get, "/a/b", noop);

    bool result = router.match(Method::Get, "/a").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchPathLongerThanRegisteredRejects) {
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/:x", noop);

    bool result = router.match(Method::Get, "/a/b").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchUnregisteredPathRejects) {
    router.add(Method::Get, "/a", noop);

    bool result = router.match(Method::Get, "/b").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchDifferentMethodReject) {
    router.add(Method::Get, "/a", noop);

    bool result = router.match(Method::Post, "/a").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchEmptyRouterRejects) {
    bool result = router.match(Method::Get, "/a").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchParamSegmentMatchesAnyValue) {
    router.add(Method::Get, "/:x", noop);

    bool result = router.match(Method::Get, "/a").has_value();
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchTreatsPatternSyntaxAsLiteralTargetValues) {
    router.add(Method::Get, "/:id", noop);
    router.add(Method::Get, "/a/:id/b", noop);

    const Router& read_only = router;
    for (const auto value : {":", ":name", "*", "*rest"}) {
        SCOPED_TRACE(value);
        EXPECT_TRUE(read_only.match(Method::Get, std::string("/") + value));
        EXPECT_TRUE(read_only.match(Method::Get, std::string("/a/") + value + "/b"));
    }
    EXPECT_TRUE(read_only.contains(Method::Get, "/:id"));
    EXPECT_FALSE(read_only.contains(Method::Get, "/:"));
    EXPECT_FALSE(read_only.contains(Method::Get, "/:name"));
    EXPECT_EQ(read_only.size(), 2u);
}

TEST_F(RouterTest, MatchStaticSegmentTakesPriorityOverParam) {
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Post, "/:x/b", noop);

    // Get is served by the static route. Post has no static handler, so it
    // falls back to the param route.
    bool get_result = router.match(Method::Get, "/a/b").has_value();
    bool post_result = router.match(Method::Post, "/a/b").has_value();
    EXPECT_TRUE(get_result);
    EXPECT_TRUE(post_result);
}

TEST_F(RouterTest, MatchBacktracksFromStaticSegmentToParam) {
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Get, "/:x/c", noop);

    bool result = router.match(Method::Get, "/a/c").has_value();
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchBacktracksMultipleLevels) {
    router.add(Method::Get, "/a/b/x", noop);
    router.add(Method::Get, "/:p/b/y", noop);

    bool result = router.match(Method::Get, "/a/b/y").has_value();
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchParamSegmentBetweenStaticSegmentsMatches) {
    router.add(Method::Get, "/a/:x/c", noop);

    bool result = router.match(Method::Get, "/a/b/c").has_value();
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchParamSegmentWithWrongStaticTailRejects) {
    router.add(Method::Get, "/a/:x/c", noop);

    bool result = router.match(Method::Get, "/a/b/d").has_value();
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchFallsBackToParamWhenStaticNodeHasNoRoute) {
    router.add(Method::Get, "/a/c", noop);
    router.add(Method::Get, "/:x", noop);

    // The static "a" node exists but is only an intermediate node, so "/:x"
    // serves the target.
    bool result = router.match(Method::Get, "/a").has_value();
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchRootAndStaticRoutesCaptureNoParams) {
    router.add(Method::Get, "/", noop);
    router.add(Method::Get, "/users/new", noop);

    for (const auto target : {"/", "/users/new"}) {
        SCOPED_TRACE(target);
        std::vector<std::string> values;
        ASSERT_NE(match_with_params(Method::Get, target, values), nullptr);
        EXPECT_TRUE(values.empty());
    }
}

TEST_F(RouterTest, MatchResultHoldsRouteAndCapturedValues) {
    router.add(Method::Get, "/users/:id/posts/:post", noop);

    auto result = router.match(Method::Get, "/users/42/posts/7");

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result->route, nullptr);
    EXPECT_EQ(result->route->param_ids, (std::vector<std::string>{"id", "post"}));
    EXPECT_EQ(result->param_vals, (std::vector<std::string>{"42", "7"}));
}

TEST_F(RouterTest, MatchResultCapturesOutliveTheTarget) {
    router.add(Method::Get, "/users/:id", noop);

    auto result = [&] {
        std::string target = "/users/42";
        return router.match(Method::Get, target);
    }();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->param_vals, (std::vector<std::string>{"42"}));
}

TEST_F(RouterTest, MatchResultForStaticRouteHasNoCaptures) {
    router.add(Method::Get, "/users/new", noop);

    auto result = router.match(Method::Get, "/users/new");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->route->param_ids.empty());
    EXPECT_TRUE(result->param_vals.empty());
}

TEST_F(RouterTest, MatchResultRunsTheRegisteredHandler) {
    int calls = 0;
    router.add(Method::Get, "/a", [&](Request&, Response&) { ++calls; });

    auto result = router.match(Method::Get, "/a");

    ASSERT_TRUE(result.has_value());
    Request req;
    Response res{};
    result->route->handler(req, res);
    EXPECT_EQ(calls, 1);
}

TEST_F(RouterTest, MatchCapturesParamsInPathOrder) {
    router.add(Method::Get, "/users/:id/posts/:post", noop);

    std::vector<std::string> values;
    auto* node = match_with_params(Method::Get, "/users/42/posts/7", values);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(values, (std::vector<std::string>{"42", "7"}));
    ASSERT_NE(node->get_route(Method::Get), nullptr);
    EXPECT_EQ(
        node->get_route(Method::Get)->param_ids,
        (std::vector<std::string>{"id", "post"})
    );
}

TEST_F(RouterTest, MatchCapturesLiteralColonValues) {
    router.add(Method::Get, "/:first/:second", noop);

    std::vector<std::string> values;
    ASSERT_NE(match_with_params(Method::Get, "/:/:", values), nullptr);
    EXPECT_EQ(values, (std::vector<std::string>{":", ":"}));
}

TEST_F(RouterTest, MatchStaticWinnerDoesNotCaptureParamAlternative) {
    router.add(Method::Get, "/users/:id", noop);
    router.add(Method::Get, "/users/new", noop);

    std::vector<std::string> values;
    auto* node = match_with_params(Method::Get, "/users/new", values);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node, find("/users/new"));
    EXPECT_TRUE(values.empty());
}

TEST_F(RouterTest, MatchCapturesParamsForRequestedMethod) {
    router.add(Method::Get, "/a/:id", noop);
    router.add(Method::Post, "/:prefix/:name", noop);

    std::vector<std::string> get_values;
    auto* get_node = match_with_params(Method::Get, "/a/42", get_values);
    ASSERT_NE(get_node, nullptr);
    EXPECT_EQ(get_values, (std::vector<std::string>{"42"}));
    EXPECT_EQ(
        get_node->get_route(Method::Get)->param_ids,
        (std::vector<std::string>{"id"})
    );

    // The static branch captures "42" before failing to find a Post handler.
    std::vector<std::string> post_values;
    auto* post_node = match_with_params(Method::Post, "/a/42", post_values);
    ASSERT_NE(post_node, nullptr);
    EXPECT_EQ(post_values, (std::vector<std::string>{"a", "42"}));
    EXPECT_EQ(
        post_node->get_route(Method::Post)->param_ids,
        (std::vector<std::string>{"prefix", "name"})
    );
}

TEST_F(RouterTest, MatchBacktrackingDiscardsFailedBranchCaptures) {
    router.add(Method::Get, "/a/:discard/:also_discard/end", noop);
    router.add(Method::Get, "/:kept/b/:tail/done", noop);

    std::vector<std::string> values;
    auto* node = match_with_params(Method::Get, "/a/b/c/done", values);

    ASSERT_NE(node, nullptr);
    EXPECT_EQ(values, (std::vector<std::string>{"a", "c"}));
    EXPECT_EQ(
        node->get_route(Method::Get)->param_ids,
        (std::vector<std::string>{"kept", "tail"})
    );
}

TEST_F(RouterTest, MatchBacktrackingPreservesSharedPrefixCapture) {
    router.add(Method::Get, "/:outer/a/:discard/end", noop);
    router.add(Method::Get, "/:outer/:inner/:tail/done", noop);

    std::vector<std::string> values;
    ASSERT_NE(match_with_params(Method::Get, "/root/a/value/done", values), nullptr);
    EXPECT_EQ(values, (std::vector<std::string>{"root", "a", "value"}));
}

TEST_F(RouterTest, MatchFailureLeavesNoParamCaptures) {
    router.add(Method::Get, "/a/:id/:tail/done", noop);

    for (const auto target :
         {"/missing", "/a/1", "/a/1/2/missing", "/a/1/2/done/extra"}) {
        SCOPED_TRACE(target);
        std::vector<std::string> values;
        EXPECT_EQ(match_with_params(Method::Get, target, values), nullptr);
        EXPECT_TRUE(values.empty());
    }

    std::vector<std::string> values;
    EXPECT_EQ(match_with_params(Method::Post, "/a/1/2/done", values), nullptr);
    EXPECT_TRUE(values.empty());
}

// Router::remove()

TEST_F(RouterTest, RemoveOnEmptyRouterIsNoOp) {
    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 0u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);
}

TEST_F(RouterTest, RemoveInvalidPathIsNoOp) {
    router.add(Method::Get, "/a", noop);

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        EXPECT_TRUE(a);
    }
}

TEST_F(RouterTest, RemoveNonExistentPathIsNoOp) {
    router.add(Method::Get, "/a", noop);

    for (const auto path : {"/b", "/a/b"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        EXPECT_TRUE(a);

        auto missing = router.contains(Method::Get, path);
        EXPECT_FALSE(missing);
    }
}

TEST_F(RouterTest, RemoveNonExistentMethodIsNoOp) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);

    router.remove(second, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_FALSE(second_a);
}

TEST_F(RouterTest, RemoveConservesOtherMethods) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);
    router.add(second, "/a", noop);

    router.remove(first, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_FALSE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
}

TEST_F(RouterTest, RemoveClearsRouteOnEmptyMethods) {
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Get, "/b", noop);

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_TRUE(b);

    router.add(Method::Get, "/a", noop);
    EXPECT_EQ(router.size(), 3u);
    a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, RemoveConservesInternalNode) {
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Get, "/a/c", noop);

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto a_c = router.contains(Method::Get, "/a/c");
    EXPECT_TRUE(a_c);
}

TEST_F(RouterTest, RemoveDeletesChildNodeIfNoMethods) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);
    router.add(first, "/a/b", noop);
    router.add(second, "/a/b", noop);
    router.add(first, "/a/c", noop);

    router.remove(first, "/a/b");
    EXPECT_EQ(router.size(), 3u);
    auto first_a_b = router.contains(first, "/a/b");
    EXPECT_FALSE(first_a_b);

    auto second_a_b = router.contains(second, "/a/b");
    EXPECT_TRUE(second_a_b);

    router.remove(second, "/a/b");
    EXPECT_EQ(router.size(), 2u);
    first_a_b = router.contains(first, "/a/b");
    EXPECT_FALSE(first_a_b);

    second_a_b = router.contains(second, "/a/b");
    EXPECT_FALSE(second_a_b);

    // The "b" node is gone, so the lookup stops at its parent.
    EXPECT_EQ(find("/a/b"), find("/a"));

    auto a = router.contains(first, "/a");
    EXPECT_TRUE(a);

    auto a_c = router.contains(first, "/a/c");
    EXPECT_TRUE(a_c);
}

TEST_F(RouterTest, RemoveParamRouteClearsRoute) {
    router.add(Method::Get, "/a/:x", noop);

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 0u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveRequiresRegisteredParamNames) {
    router.add(Method::Get, "/a/:x/:y", noop);

    for (const auto pattern : {"/a/:other/:y", "/a/:x/:other", "/a/:/:y"}) {
        SCOPED_TRACE(pattern);
        router.remove(Method::Get, pattern);
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a/:x/:y"));
        EXPECT_TRUE(router.match(Method::Get, "/a/1/2"));
    }
}

TEST_F(RouterTest, RemoveRootRoutePreservesDescendants) {
    router.add(Method::Get, "/", noop);
    router.add(Method::Get, "/a/:id", noop);

    router.remove(Method::Get, "/");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/"));
    EXPECT_FALSE(router.match(Method::Get, "/"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:id"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingAtFirstSegmentIsNoOp) {
    router.add(Method::Get, "/x", noop);

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/x"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingAtParamBranchIsNoOp) {
    router.add(Method::Get, "/x", noop);

    router.remove(Method::Get, "/:id/b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/x"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingInTheMiddleIsNoOp) {
    router.add(Method::Get, "/a/b", noop);

    router.remove(Method::Get, "/a/x/c");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
}

TEST_F(RouterTest, RemoveStaticRouteConservesSiblingParamRoute) {
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Get, "/a/:x", noop);

    router.remove(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/b"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveParamRouteConservesSiblingStaticRoute) {
    router.add(Method::Get, "/a/b", noop);
    router.add(Method::Get, "/a/:x", noop);

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveParamRoutePrunesParamNodeAndRoutelessAncestors) {
    router.add(Method::Get, "/a/:x", noop);

    router.remove(Method::Get, "/a/:x");

    // Both the param node and the "a" node that held it are gone.
    EXPECT_EQ(find("/")->get_child("a"), nullptr);
    EXPECT_EQ(find("/")->get_child(":"), nullptr);
}

TEST_F(RouterTest, RemoveParamRoutePruningStopsAtAncestorWithRoute) {
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a/:x/c", noop);

    router.remove(Method::Get, "/a/:x/c");

    // The param node and its "c" child are pruned, but "a" still holds a route.
    Node* a = find("/a");
    EXPECT_NE(a, find("/"));
    EXPECT_EQ(a->get_child(":"), nullptr);
    EXPECT_TRUE(router.contains(Method::Get, "/a"));
}

TEST_F(RouterTest, RemoveParamRouteConservesParamNodeWithChildRoute) {
    router.add(Method::Get, "/a/:x", noop);
    router.add(Method::Get, "/a/:x/c", noop);

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:x/c"));
    EXPECT_NE(find("/a")->get_child(":"), nullptr);
}

TEST_F(RouterTest, RemovePrunesChainOfRoutelessNodes) {
    router.add(Method::Get, "/a/b/c", noop);
    router.add(Method::Get, "/d", noop);

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    auto d = router.contains(Method::Get, "/d");
    EXPECT_TRUE(d);

    // The whole "a/b/c" chain is gone, so the lookup stops at the root.
    EXPECT_EQ(find("/a/b/c"), find("/"));
    EXPECT_EQ(find("/")->get_child("a"), nullptr);

    router.add(Method::Get, "/a/b/c", noop);
    EXPECT_EQ(router.size(), 2u);
    a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_TRUE(a_b_c);
}

TEST_F(RouterTest, RemovePruningStopsAtAncestorWithRoute) {
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a/b/c", noop);

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);

    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    // "c" and "b" are pruned, but "a" still holds a route.
    Node* node = find("/a/b/c");
    EXPECT_EQ(node, find("/a"));
    EXPECT_EQ(node->get_child("b"), nullptr);
}

TEST_F(RouterTest, RemoveLastRouteLeavesRouterUsable) {
    router.add(Method::Get, "/a/b", noop);
    router.remove(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 0u);
    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);

    // Only the root is left.
    Node* root = find("/a/b");
    EXPECT_EQ(root, find("/"));
    EXPECT_EQ(root->get_child("a"), nullptr);

    router.add(Method::Get, "/a", noop);
    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

// Router::size()

TEST_F(RouterTest, SizeCountsDuplicateRouteOnce) {
    router.add(Method::Get, "/a", noop);
    router.add(Method::Get, "/a", noop);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, SizeCountsEachMethodOnSamePath) {
    const auto first = Method::Get;
    const auto second = Method::Post;
    router.add(first, "/a", noop);
    router.add(second, "/a", noop);

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
}
