#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <thread>

#include <drogon/drogon.h>

import app.web.server;
import app.version;

using namespace app::web;

namespace {

constexpr uint16_t kTestPort = 18081;
const std::string kBaseUrl = "http://127.0.0.1:" + std::to_string(kTestPort);

std::filesystem::path CreateTempWebRoot() {
    auto dir = std::filesystem::temp_directory_path() / "ffc_web_test";
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "index.html") << "<html><body>FaceFusionCpp Web Test</body></html>";
    return dir;
}

std::thread StartServerInThread(const std::string& web_root) {
    std::thread server_thread([web_root]() {
        run_server({.host = "127.0.0.1", .port = kTestPort, .web_root = web_root});
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    return server_thread;
}

std::string Get(const std::string& path) {
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath(path);
    std::string body;
    std::promise<void> done;
    client->sendRequest(req, [&](drogon::ReqResult, const drogon::HttpResponsePtr& resp) {
        if (resp) { body = std::string(resp->getBody()); }
        done.set_value();
    });
    done.get_future().wait();
    return body;
}

} // namespace

TEST(WebServerTest, ServesHealthAndStaticContent) {
    auto web_root = CreateTempWebRoot();
    auto server_thread = StartServerInThread(web_root.string());

    // 1. /api/health
    auto health = Get("/api/health");
    EXPECT_NE(health.find("\"status\":\"ok\""), std::string::npos);
    EXPECT_NE(health.find("\"version\":\"" + std::string(app::version::version()) + "\""),
              std::string::npos);

    // 2. static index.html
    auto index = Get("/");
    EXPECT_NE(index.find("FaceFusionCpp Web Test"), std::string::npos);

    // 3. stop the event loop and wait for the server thread to exit cleanly
    drogon::app().quit();
    server_thread.join();
}
