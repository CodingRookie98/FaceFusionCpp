#include <gtest/gtest.h>
#include <string>

import app.web.server;

using namespace app::web;

TEST(WebServerHealthTest, HealthJsonContainsOkAndVersion) {
    auto body = health_json();
    EXPECT_NE(body.find("\"status\":\"ok\""), std::string::npos);
    EXPECT_NE(body.find("\"version\""), std::string::npos);
    EXPECT_NE(body.find("0.34"), std::string::npos); // version prefix sanity
}
