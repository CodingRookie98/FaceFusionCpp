#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <random>
#include <string>
#include <thread>

#include <drogon/drogon.h>

import app.web.server;
import app.web.task_manager;
import app.version;
import config.task;
import services.pipeline.runner;

using namespace app::web;

namespace {

// ctest runs each gtest case as a separate process; derive a per-process
// port to avoid TIME_WAIT bind conflicts between cases.
// ctest runs each gtest case as a separate process; pick a random port to
// avoid TIME_WAIT/PID-reuse bind conflicts (fixed 1808x ports flaked).
static uint16_t RandomTestPort() {
    auto seed = static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::mt19937 gen(seed);
    return static_cast<uint16_t>(20000 + (gen() % 20000)); // 20000-39999
}
const uint16_t kTestPort = RandomTestPort();
const std::string kBaseUrl = "http://127.0.0.1:" + std::to_string(kTestPort);

/// Minimal fake executor: reports progress then succeeds
class FakeExecutor : public ITaskExecutor {
public:
    int run(const config::TaskConfig&, const services::pipeline::ProgressCallback& cb) override {
        services::pipeline::TaskProgress p;
        p.current_frame = 1;
        p.total_frames = 1;
        p.fps = 30.0;
        cb(p);
        return 0;
    }
    void cancel() override {}
};

std::filesystem::path CreateTempWebRoot() {
    auto dir = std::filesystem::temp_directory_path() / "ffc_web_test";
    std::filesystem::create_directories(dir);
    std::ofstream(dir / "index.html") << "<html><body>FaceFusionCpp Web Test</body></html>";
    return dir;
}

std::thread StartServerInThread(const std::string& web_root) {
    auto tasks = std::make_shared<TaskManager>(std::make_shared<FakeExecutor>());
    std::thread server_thread([web_root, tasks]() {
        run_server({.host = "127.0.0.1", .port = kTestPort, .web_root = web_root},
                   {.tasks = tasks, .app_config = nullptr});
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
