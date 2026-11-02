#include "nexora/http/headers.hpp"

#include <gtest/gtest.h>

using namespace std;
using namespace nexora::http;

TEST(Headers, NulloptOnNonExistentHeader) {
    Headers h;

    h.add("Host", "example.com");

    EXPECT_FALSE(h.get("Invalid").has_value());

    Headers empty;

    EXPECT_FALSE(empty.get("Host").has_value());
}

TEST(Headers, AddRejectsDuplicates) {
    Headers h;

    EXPECT_TRUE(h.add("Accept", "application/json"));
    EXPECT_TRUE(h.add("Host", "example.com"));
    EXPECT_FALSE(h.add("Accept", "text/html"));

    EXPECT_EQ(h.get("Accept"), "application/json");
    EXPECT_EQ(h.fields.size(), 2u);
}

TEST(Headers, SetReplacesExistingValue) {
    Headers h;

    h.add("Accept", "application/json");
    h.set("accept", "text/plain");

    EXPECT_EQ(h.get("Accept"), "text/plain");
    EXPECT_EQ(h.fields.size(), 1u);
}

// Field names are case-insensitive (RFC 9110 §5.1).
using CaseInsensitiveCase = tuple<string, string>;

class CaseInsensitiveLookupTest : public ::testing::TestWithParam<CaseInsensitiveCase> {};

TEST_P(CaseInsensitiveLookupTest, MatchesRegardlessOfCase) {
    auto [stored, lookup] = GetParam();
    Headers h;

    h.add(stored, "text/html");

    auto get_result = h.get(lookup);

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "text/html");
}

TEST_P(CaseInsensitiveLookupTest, AddRejectsDuplicateRegardlessOfCase) {
    auto [stored, lookup] = GetParam();
    Headers h;

    h.add(stored, "a");

    EXPECT_FALSE(h.add(lookup, "b"));
    EXPECT_EQ(h.fields.size(), 1u);
    EXPECT_EQ(h.get(lookup), "a");
}

TEST_P(CaseInsensitiveLookupTest, RemovesRegardlessOfCase) {
    auto [stored, lookup] = GetParam();
    Headers h;

    h.add(stored, "text/html");
    h.remove(lookup);

    EXPECT_TRUE(h.fields.empty());
}

INSTANTIATE_TEST_SUITE_P(
    Headers,
    CaseInsensitiveLookupTest,
    ::testing::Values(
        CaseInsensitiveCase{"Content-Type", "Content-Type"},
        CaseInsensitiveCase{"Content-Type", "content-type"},
        CaseInsensitiveCase{"Content-Type", "CONTENT-TYPE"},
        CaseInsensitiveCase{"Content-Type", "cOnTeNt-TyPe"},
        CaseInsensitiveCase{"content-type", "Content-Type"},
        CaseInsensitiveCase{"CONTENT-TYPE", "content-type"}
    )
);

TEST(Headers, RemoveLeavesOtherFields) {
    Headers h;

    h.add("Host", "example.com");
    h.add("Accept", "application/json");
    h.add("Connection", "close");

    h.remove("Accept");

    EXPECT_EQ(h.fields.size(), 2u);
    EXPECT_FALSE(h.get("Accept").has_value());
    EXPECT_TRUE(h.get("Host").has_value());
    EXPECT_TRUE(h.get("Connection").has_value());
}

TEST(Headers, EmptyValueIsDistinctFromMissing) {
    Headers h;

    h.add("X-Empty", "");

    auto get_result = h.get("X-Empty");

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "");
}

TEST(Headers, SubscriptMatchesGet) {
    Headers h;

    h.add("Accept", "application/json");

    EXPECT_EQ(h["Accept"], h.get("Accept"));
    EXPECT_EQ(h["Invalid"], h.get("Invalid"));
}
