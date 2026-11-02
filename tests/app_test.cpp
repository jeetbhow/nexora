#include "nexora/app.hpp"

#include <gtest/gtest.h>

using namespace nexora;
using namespace nexora::http;

TEST(App, UnmatchedRoutesDoNotInvokeHandler) {
    App app;
    int calls = 0;
    app.get("/users", [&](Request&, Response&) { ++calls; });

    app.route(Method::Get, "/missing");
    app.route(Method::Post, "/users");
    EXPECT_EQ(calls, 0);
}
