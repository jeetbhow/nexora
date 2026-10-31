#include <gtest/gtest.h>

#include "http/router.hpp"

#include <array>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace http;

constexpr std::array methods{
    Method::Get, Method::Head, Method::Post, Method::Put,
    Method::Delete, Method::Patch, Method::Options,
};

class RouterTest : public testing::Test {
protected:
    // Walk static branches, stopping at the last existing node.
    RouterNode* find(std::string_view path) {
        auto* node = &router.root_;
        for (auto part : path.substr(1) | std::views::split('/')) {
            auto* child = node->get_child(std::string_view(part.begin(), part.end()));
            if (!child) {
                break;
            }
            node = child;
        }
        return node;
    }

    Router router;
};

class RouterMethodTest : public RouterTest,
                         public testing::WithParamInterface<Method> {};

using MethodPair = std::pair<Method, Method>;

std::vector<MethodPair> distinct_method_pairs() {
    std::vector<MethodPair> pairs;
    for (const auto first : methods) {
        for (const auto second : methods) {
            if (first != second) {
                pairs.emplace_back(first, second);
            }
        }
    }
    return pairs;
}

class RouterMethodPairTest : public RouterTest,
                             public testing::WithParamInterface<MethodPair> {};

// Router::add()

TEST_F(RouterTest, AddInvalidPathIsNoOp) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.add(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, AddRejectsParamSegmentWithDifferentNameAtSamePosition) {
    router.add(Method::Get, "/a/:c");
    router.add(Method::Get, "/a/:b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_FALSE(router.contains(Method::Get, "/a/:b"));
}

TEST_F(RouterTest, AddAllowsDifferentParamNameWithDifferentMethod) {
    router.add(Method::Get, "/a/:c");
    router.add(Method::Post, "/a/:b");

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_TRUE(router.contains(Method::Post, "/a/:b"));

    // Each handler keeps its own param names.
    EXPECT_FALSE(router.contains(Method::Get, "/a/:b"));
    EXPECT_FALSE(router.contains(Method::Post, "/a/:c"));
}

TEST_F(RouterTest, AddAllowsDifferentParamNamesInSharedPrefix) {
    router.add(Method::Get, "/a/:b/x");
    router.add(Method::Get, "/a/:c/y");

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:b/x"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c/y"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1/x"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1/y"));
}

TEST_F(RouterTest, AddRejectsConflictingParamNameAtRoot) {
    router.add(Method::Get, "/:a");
    router.add(Method::Get, "/:b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/:b"));
}

TEST_F(RouterTest, AddAllowsSameParamNameWithDifferentMethod) {
    router.add(Method::Get, "/a/:c");
    router.add(Method::Post, "/a/:c");

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/:c"));
    EXPECT_TRUE(router.contains(Method::Post, "/a/:c"));
}

TEST_F(RouterTest, AddRejectsParamSegmentWithEmptyName) {
    router.add(Method::Get, "/:");

    EXPECT_EQ(router.size(), 0u);
    EXPECT_FALSE(router.contains(Method::Get, "/:"));
}

TEST_F(RouterTest, WildcardPatternsAreRejectedWithoutChangingExistingRoutes) {
    router.add(Method::Get, "/");
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/b");

    for (const auto pattern : {"/*", "/a/*", "/a/*rest", "/a/*/b", "/new/*"}) {
        SCOPED_TRACE(pattern);
        // Post would create a distinct handler if the wildcard were silently skipped.
        router.add(Method::Post, pattern);
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

TEST_F(RouterTest, AddAllowsStaticAndParamSegmentAtSamePosition) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/:x/b");

    EXPECT_EQ(router.size(), 2u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
    EXPECT_TRUE(router.contains(Method::Get, "/:x/b"));
}

TEST_P(RouterMethodTest, AddRegistersSingleRoute) {
    const auto method = GetParam();
    router.add(method, "/a");

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(method, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, AddRegistersTwoRoutesWithSameMethod) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/b");

    EXPECT_EQ(router.size(), 2u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_TRUE(b);
}

TEST_P(RouterMethodPairTest, AddRegistersTwoRoutesWithDifferentMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/b");

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
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.contains(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, ContainsReturnsFalseForUnaddedRoute) {
    router.add(Method::Get, "/a");

    auto b = router.contains(Method::Get, "/b");
    EXPECT_FALSE(b);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);
}

TEST_F(RouterTest, ContainsReturnsFalseForIntermediateNode) {
    router.add(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);
}

TEST_F(RouterTest, ContainsParamRouteOnlyForRegisteredPattern) {
    router.add(Method::Get, "/:x");

    EXPECT_TRUE(router.contains(Method::Get, "/:x"));
    EXPECT_FALSE(router.contains(Method::Get, "/a"));
}

TEST_P(RouterMethodPairTest, ContainsReturnsFalseForDifferentMethod) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");

    auto a = router.contains(second, "/a");
    EXPECT_FALSE(a);
}

// Router::match()

TEST_F(RouterTest, MatchInvalidPathReturnsFalse) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a", "//", "/a/"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.match(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, MatchRegisteredPathMatches) {
    router.add(Method::Get, "/a");

    bool result = router.match(Method::Get, "/a");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchRecursivePathMatches) {
    router.add(Method::Get, "/a/b");

    bool result = router.match(Method::Get, "/a/b");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchRootPathMatches) {
    router.add(Method::Get, "/");

    bool result = router.match(Method::Get, "/");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchIntermediateNodeWithoutRouteRejects) {
    router.add(Method::Get, "/a/b");

    bool result = router.match(Method::Get, "/a");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchPathLongerThanRegisteredRejects) {
    router.add(Method::Get, "/a");

    bool result = router.match(Method::Get, "/a/b");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchUnregisteredPathRejects) {
    router.add(Method::Get, "/a");

    bool result = router.match(Method::Get, "/b");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchDifferentMethodReject) {
    router.add(Method::Get, "/a");

    bool result = router.match(Method::Post, "/a");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchEmptyRouterRejects) {
    bool result = router.match(Method::Get, "/a");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchParamSegmentMatchesAnyValue) {
    router.add(Method::Get, "/:x");

    bool result = router.match(Method::Get, "/a");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchTreatsPatternSyntaxAsLiteralTargetValues) {
    router.add(Method::Get, "/:id");
    router.add(Method::Get, "/a/:id/b");

    Router& read_only = router;
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

TEST_F(RouterTest, MatchParamSegmentFollowedByStaticSegmentMatches) {
    router.add(Method::Get, "/:x/b");

    bool result = router.match(Method::Get, "/a/b");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchStaticSegmentTakesPriorityOverParam) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Post, "/:x/b");

    // Get is served by the static route. Post has no static handler, so it falls back to the
    // param route.
    bool get_result = router.match(Method::Get, "/a/b");
    bool post_result = router.match(Method::Post, "/a/b");
    EXPECT_TRUE(get_result);
    EXPECT_TRUE(post_result);
}

TEST_F(RouterTest, MatchBacktracksFromStaticSegmentToParam) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/:x/c");

    bool result = router.match(Method::Get, "/a/c");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchParamSegmentDoesNotSpanMultipleSegments) {
    router.add(Method::Get, "/:x");

    bool result = router.match(Method::Get, "/a/b");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchMultipleParamSegmentsMatch) {
    router.add(Method::Get, "/:x/:y");

    bool result = router.match(Method::Get, "/a/b");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchBacktracksMultipleLevels) {
    router.add(Method::Get, "/a/b/x");
    router.add(Method::Get, "/:p/b/y");

    bool result = router.match(Method::Get, "/a/b/y");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchParamSegmentBetweenStaticSegmentsMatches) {
    router.add(Method::Get, "/a/:x/c");

    bool result = router.match(Method::Get, "/a/b/c");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchParamSegmentWithWrongStaticTailRejects) {
    router.add(Method::Get, "/a/:x/c");

    bool result = router.match(Method::Get, "/a/b/d");
    EXPECT_FALSE(result);
}

TEST_F(RouterTest, MatchFallsBackToParamWhenStaticNodeHasNoRoute) {
    router.add(Method::Get, "/a/c");
    router.add(Method::Get, "/:x");

    // The static "a" node exists but is only an intermediate node, so "/:x" serves the target.
    bool result = router.match(Method::Get, "/a");
    EXPECT_TRUE(result);
}

TEST_F(RouterTest, MatchIntermediateNodeWithoutParamRouteRejects) {
    router.add(Method::Get, "/a/c");

    bool result = router.match(Method::Get, "/a");
    EXPECT_FALSE(result);
}

// Router::remove()

TEST_F(RouterTest, RemoveOnEmptyRouterIsNoOp) {
    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 0u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);
}

TEST_F(RouterTest, RemoveInvalidPathIsNoOp) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        EXPECT_TRUE(a);
    }
}

TEST_F(RouterTest, RemoveNonExistentPathIsNoOp) {
    router.add(Method::Get, "/a");

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

TEST_P(RouterMethodPairTest, RemoveNonExistentMethodIsNoOp) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");

    router.remove(second, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_FALSE(second_a);
}

TEST_P(RouterMethodPairTest, RemoveConservesOtherMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/a");

    router.remove(first, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_FALSE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
}

TEST_F(RouterTest, RemoveClearsRouteOnEmptyMethods) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/b");

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_TRUE(b);

    router.add(Method::Get, "/a");
    EXPECT_EQ(router.size(), 3u);
    a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, RemoveConservesInternalNode) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/a/c");

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto a_c = router.contains(Method::Get, "/a/c");
    EXPECT_TRUE(a_c);
}

TEST_P(RouterMethodPairTest, RemoveDeletesChildNodeIfNoMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(first, "/a/b");
    router.add(second, "/a/b");
    router.add(first, "/a/c");

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
    router.add(Method::Get, "/a/:x");

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 0u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveRequiresRegisteredParamNames) {
    router.add(Method::Get, "/a/:x/:y");

    for (const auto pattern : {"/a/:other/:y", "/a/:x/:other", "/a/:/:y"}) {
        SCOPED_TRACE(pattern);
        router.remove(Method::Get, pattern);
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a/:x/:y"));
        EXPECT_TRUE(router.match(Method::Get, "/a/1/2"));
    }
}

TEST_F(RouterTest, RemoveRootRoutePreservesDescendants) {
    router.add(Method::Get, "/");
    router.add(Method::Get, "/a/:id");

    router.remove(Method::Get, "/");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/"));
    EXPECT_FALSE(router.match(Method::Get, "/"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:id"));
    EXPECT_TRUE(router.match(Method::Get, "/a/1"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingAtFirstSegmentIsNoOp) {
    router.add(Method::Get, "/x");

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/x"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingAtParamBranchIsNoOp) {
    router.add(Method::Get, "/x");

    router.remove(Method::Get, "/:id/b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/x"));
}

TEST_F(RouterTest, RemoveMultiSegmentPathMissingInTheMiddleIsNoOp) {
    router.add(Method::Get, "/a/b");

    router.remove(Method::Get, "/a/x/c");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
}

TEST_F(RouterTest, RemoveStaticRouteConservesSiblingParamRoute) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/a/:x");

    router.remove(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/b"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveParamRouteConservesSiblingStaticRoute) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/a/:x");

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_TRUE(router.contains(Method::Get, "/a/b"));
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
}

TEST_F(RouterTest, RemoveParamRoutePrunesParamNodeAndRoutelessAncestors) {
    router.add(Method::Get, "/a/:x");

    router.remove(Method::Get, "/a/:x");

    // Both the param node and the "a" node that held it are gone.
    EXPECT_EQ(find("/")->get_child("a"), nullptr);
    EXPECT_EQ(find("/")->get_param_child(), nullptr);
}

TEST_F(RouterTest, RemoveParamRoutePruningStopsAtAncestorWithRoute) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/:x/c");

    router.remove(Method::Get, "/a/:x/c");

    // The param node and its "c" child are pruned, but "a" still holds a route.
    RouterNode* a = find("/a");
    EXPECT_NE(a, find("/"));
    EXPECT_EQ(a->get_param_child(), nullptr);
    EXPECT_TRUE(router.contains(Method::Get, "/a"));
}

TEST_F(RouterTest, RemoveParamRouteConservesParamNodeWithChildRoute) {
    router.add(Method::Get, "/a/:x");
    router.add(Method::Get, "/a/:x/c");

    router.remove(Method::Get, "/a/:x");

    EXPECT_EQ(router.size(), 1u);
    EXPECT_FALSE(router.contains(Method::Get, "/a/:x"));
    EXPECT_TRUE(router.contains(Method::Get, "/a/:x/c"));
    EXPECT_NE(find("/a")->get_param_child(), nullptr);
}

TEST_F(RouterTest, RemovePrunesChainOfRoutelessNodes) {
    router.add(Method::Get, "/a/b/c");
    router.add(Method::Get, "/d");

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    auto d = router.contains(Method::Get, "/d");
    EXPECT_TRUE(d);

    // The whole "a/b/c" chain is gone, so the lookup stops at the root.
    EXPECT_EQ(find("/a/b/c"), find("/"));
    EXPECT_EQ(find("/")->get_child("a"), nullptr);

    router.add(Method::Get, "/a/b/c");
    EXPECT_EQ(router.size(), 2u);
    a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_TRUE(a_b_c);
}

TEST_F(RouterTest, RemovePruningStopsAtAncestorWithRoute) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/b/c");

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);

    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    // "c" and "b" are pruned, but "a" still holds a route.
    RouterNode* node = find("/a/b/c");
    EXPECT_EQ(node, find("/a"));
    EXPECT_EQ(node->get_child("b"), nullptr);
}

TEST_F(RouterTest, RemoveLastRouteLeavesRouterUsable) {
    router.add(Method::Get, "/a/b");
    router.remove(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 0u);
    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);

    // Only the root is left.
    RouterNode* root = find("/a/b");
    EXPECT_EQ(root, find("/"));
    EXPECT_EQ(root->get_child("a"), nullptr);

    router.add(Method::Get, "/a");
    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

// Router::size()

TEST_F(RouterTest, SizeCountsDuplicateRouteOnce) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a");

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_P(RouterMethodPairTest, SizeCountsEachMethodOnSamePath) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/a");

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
}

INSTANTIATE_TEST_SUITE_P(
    AllMethods,
    RouterMethodTest,
    testing::ValuesIn(methods),
    [](const testing::TestParamInfo<Method>& info) {
        return std::string(http::to_string(info.param));
    }
);

INSTANTIATE_TEST_SUITE_P(
    DistinctMethods,
    RouterMethodPairTest,
    testing::ValuesIn(distinct_method_pairs()),
    [](const testing::TestParamInfo<MethodPair>& info) {
        return std::string(http::to_string(info.param.first)) + "_" +
               std::string(http::to_string(info.param.second));
    }
);
