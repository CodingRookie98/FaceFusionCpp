#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include <drogon/drogon.h>
#include <drogon/WebSocketClient.h>
#include <nlohmann/json.hpp>

import app.web.server;
import app.web.task_manager;
import app.web.task_types;
import config.task;
import services.pipeline.runner;

using namespace app::web;
using json = nlohmann::json;

namespace {

// ctest runs each gtest case as a separate process; derive a per-process
// port to avoid TIME_WAIT bind conflicts between cases.
#ifdef _WIN32
#include <process.h>
#define FFC_GETPID _getpid
#else
#include <unistd.h>
#define FFC_GETPID getpid
#endif
const uint16_t kTestPort = static_cast<uint16_t>(18080 + (FFC_GETPID() % 1000));
const std::string kBaseUrl = "http://127.0.0.1:" + std::to_string(kTestPort);

/// Fake executor: emits a progress callback sequence then succeeds
class FakeExecutor : public ITaskExecutor {
public:
    int run(const config::TaskConfig&, const services::pipeline::ProgressCallback& cb) override {
        for (int i = 1; i <= 3; ++i) {
            services::pipeline::TaskProgress p;
            p.current_frame = static_cast<std::size_t>(i);
            p.total_frames = 3;
            p.fps = 24.0;
            cb(p);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return 0;
    }
    void cancel() override {}
};

std::shared_ptr<TaskManager> g_tasks;
std::thread g_server_thread;

void StartServer() {
    g_tasks = std::make_shared<TaskManager>(std::make_shared<FakeExecutor>());
    g_server_thread = std::thread([]() {
        run_server({.host = "127.0.0.1", .port = kTestPort, .web_root = ""},
                   {.tasks = g_tasks, .app_config = nullptr});
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
}

void StopServer() {
    drogon::app().quit();
    g_server_thread.join();
}

std::pair<int, std::string> SendRequest(drogon::HttpMethod method, const std::string& path,
                                        const std::string& body = "") {
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(method);
    req->setPath(path);
    if (!body.empty()) {
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(body);
    }
    std::pair<int, std::string> out{0, ""};
    std::promise<void> done;
    client->sendRequest(req, [&](drogon::ReqResult, const drogon::HttpResponsePtr& resp) {
        if (resp) {
            out.first = resp->getStatusCode();
            out.second = std::string(resp->getBody());
        }
        done.set_value();
    });
    done.get_future().wait();
    return out;
}

} // namespace

class WebWsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { StartServer(); }
    static void TearDownTestSuite() { StopServer(); }
};

TEST_F(WebWsTest, ReceivesProgressAndDoneMessages) {
    // Submit a task
    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks",
                                    R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":"ws_test_out"})");
    ASSERT_EQ(code, 201);
    auto id = json::parse(resp)["id"].get<std::string>();

    // Connect WS before the task finishes (fake takes ~60ms; connect immediately)
    auto wsClient =
        drogon::WebSocketClient::newWebSocketClient("127.0.0.1", kTestPort);
    std::vector<json> messages;
    std::mutex msgs_mutex;
    std::promise<void> connected;
    std::promise<void> got_done;

    std::atomic<bool> done_flag{false};
    wsClient->setMessageHandler(
        [&](std::string&& msg, const drogon::WebSocketClientPtr&,
            const drogon::WebSocketMessageType& type) {
            if (type != drogon::WebSocketMessageType::Text) { return; }
            auto j = json::parse(msg);
            {
                std::lock_guard lock(msgs_mutex);
                messages.push_back(j);
            }
            if (!done_flag.exchange(true) &&
                ((j["type"] == "done") ||
                 (j["type"] == "status" && j["status"] == "done"))) {
                got_done.set_value();
            }
        });
    wsClient->setConnectionClosedHandler([](const drogon::WebSocketClientPtr&) {});

    auto ws_req = drogon::HttpRequest::newHttpRequest();
    ws_req->setPath("/ws/tasks/" + id + "/progress");
    std::atomic<bool> connected_flag{false};
    wsClient->connectToServer(ws_req, [&](drogon::ReqResult, const drogon::HttpResponsePtr&,
                                          const drogon::WebSocketClientPtr&) {
        if (!connected_flag.exchange(true)) { connected.set_value(); }
    });
    auto connected_status =
        connected.get_future().wait_for(std::chrono::seconds(3));
    ASSERT_EQ(connected_status, std::future_status::ready);

    // Wait for done message (timeout 5s)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        std::lock_guard lock(msgs_mutex);
        for (const auto& m : messages) {
            if (m["type"] == "status" && m["status"] == "done") { got_done.set_value(); }
        }
        if (messages.size() >= 5) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::lock_guard lock(msgs_mutex);
    // Expect: status(queued/running) + 3x progress + status(done)
    int progress_count = 0;
    bool saw_done = false;
    for (const auto& m : messages) {
        if (m["type"] == "progress") {
            ++progress_count;
            EXPECT_TRUE(m.contains("frame"));
            EXPECT_TRUE(m.contains("fps"));
        } else if (m["type"] == "status" && m["status"] == "done") {
            saw_done = true;
        }
    }
    EXPECT_GE(progress_count, 1);
    EXPECT_TRUE(saw_done);
}

TEST_F(WebWsTest, UnknownTaskGetsErrorMessage) {
    auto wsClient =
        drogon::WebSocketClient::newWebSocketClient("127.0.0.1", kTestPort);
    std::promise<std::string> msg_promise;
    wsClient->setMessageHandler(
        [&](std::string&& msg, const drogon::WebSocketClientPtr&,
            const drogon::WebSocketMessageType& type) {
            if (type == drogon::WebSocketMessageType::Text) { msg_promise.set_value(msg); }
        });
    wsClient->setConnectionClosedHandler([](const drogon::WebSocketClientPtr&) {});

    std::promise<void> connected;
    std::atomic<bool> connected_flag{false};
    auto ws_req = drogon::HttpRequest::newHttpRequest();
    ws_req->setPath("/ws/tasks/nonexistent_ws_task/progress");
    wsClient->connectToServer(ws_req,
                              [&](drogon::ReqResult, const drogon::HttpResponsePtr&,
                                  const drogon::WebSocketClientPtr&) {
                                  if (!connected_flag.exchange(true)) {
                                      connected.set_value();
                                  }
                              });
    auto cstatus = connected.get_future().wait_for(std::chrono::seconds(3));
    ASSERT_EQ(cstatus, std::future_status::ready);

    auto fut = msg_promise.get_future();
    EXPECT_EQ(fut.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    auto msg = json::parse(fut.get());
    EXPECT_EQ(msg["type"], "error");
    EXPECT_EQ(msg["message"], "task not found");
}
