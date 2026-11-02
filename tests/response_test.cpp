#include "nexora/http/response.hpp"

#include <gtest/gtest.h>

using namespace std;
using namespace nexora::http;

TEST(Response, SerializesStatusLineHeadersAndBody) {
    Response res;
    res.version = Version::Http11;
    res.status = Status::NotFound;
    res.headers.set("Content-Length", "2");
    res.body = "hi";

    EXPECT_EQ(res.serialize(), "HTTP/1.1 404 Not Found\r\nContent-Length: 2\r\n\r\nhi");
}

TEST(Response, SerializesHttp10Version) {
    Response res;
    res.version = Version::Http10;
    res.status = Status::Ok;

    EXPECT_EQ(res.serialize().substr(0, 15), "HTTP/1.0 200 OK");
}
