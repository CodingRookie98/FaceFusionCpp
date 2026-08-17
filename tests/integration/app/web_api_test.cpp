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
const std::string kOutputDir = "web_api_test_output";

/// Fake executor: creates an output file (simulates a finished task)
class FakeExecutor : public ITaskExecutor {
public:
    int run(const config::TaskConfig& config, const services::pipeline::ProgressCallback& cb) override {
        if (block.load()) {
            while (!cancelled.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return 2;
        }
        std::filesystem::create_directories(config.io.output.path);
        std::ofstream(std::filesystem::path(config.io.output.path) / "result.png") << "fake-image";
        services::pipeline::TaskProgress p;
        p.current_frame = 1;
        p.total_frames = 1;
        p.fps = 30.0;
        cb(p);
        return 0;
    }
    void cancel() override { cancelled.store(true); }
    std::atomic<bool> block{false};
    std::atomic<bool> cancelled{false};
};

std::shared_ptr<TaskManager> g_tasks;
std::shared_ptr<FakeExecutor> g_executor;
std::thread g_server_thread;

void StartServer() {
    g_executor = std::make_shared<FakeExecutor>();
    g_tasks = std::make_shared<TaskManager>(g_executor);
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

/// Async HTTP request helper
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

json SubmitTask(const std::string& body) {
    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks", body);
    EXPECT_EQ(code, 201);
    return json::parse(resp);
}

} // namespace

class WebApiTest : public ::testing::Test {
protected:
    // Server lifecycle is suite-scoped: drogon::app() is a singleton and
    // cannot be cleanly run() multiple times within one process.
    static void SetUpTestSuite() { StartServer(); }
    static void TearDownTestSuite() { StopServer(); }
    void SetUp() override { std::filesystem::remove_all(kOutputDir); }
    void TearDown() override { std::filesystem::remove_all(kOutputDir); }
};

TEST_F(WebApiTest, SubmitListDetailResultFlow) {
    // 1. Submit
    auto created = SubmitTask(R"({"source_paths":["src.jpg"],"target_paths":["tgt.jpg"],"output_path":")"
                              + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();
    EXPECT_FALSE(id.empty());

    // 2. Poll until done
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    std::string status;
    while (std::chrono::steady_clock::now() < deadline) {
        auto [code, resp] = SendRequest(drogon::Get, "/api/tasks/" + id);
        ASSERT_EQ(code, 200);
        status = json::parse(resp)["status"].get<std::string>();
        if (status == "done") { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_EQ(status, "done");

    // 3. Detail includes media urls and results
    auto [code, resp] = SendRequest(drogon::Get, "/api/tasks/" + id);
    ASSERT_EQ(code, 200);
    auto detail = json::parse(resp);
    EXPECT_EQ(detail["status"], "done");
    EXPECT_EQ(detail["media"]["source"][0], "/media/" + id + "/source/0");
    ASSERT_EQ(detail["results"].size(), 1u);
    EXPECT_EQ(detail["results"][0]["name"], "result.png");

    // 4. List contains the task
    auto [lcode, lresp] = SendRequest(drogon::Get, "/api/tasks");
    ASSERT_EQ(lcode, 200);
    auto list = json::parse(lresp);
    bool found = false;
    for (const auto& t : list) {
        if (t["id"] == id) { found = true; }
    }
    EXPECT_TRUE(found);

    // 5. Result endpoint
    auto [rcode, rresp] = SendRequest(drogon::Get, "/api/tasks/" + id + "/result");
    ASSERT_EQ(rcode, 200);
    auto result = json::parse(rresp);
    ASSERT_EQ(result["files"].size(), 1u);
    EXPECT_EQ(result["files"][0]["url"], "/media/" + id + "/result/result.png");
}

TEST_F(WebApiTest, MediaEndpointServesResultFile) {
    auto created = SubmitTask(R"({"source_paths":["src.jpg"],"target_paths":["tgt.jpg"],"output_path":")"
                              + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    std::string status;
    while (std::chrono::steady_clock::now() < deadline) {
        auto [code, resp] = SendRequest(drogon::Get, "/api/tasks/" + id);
        status = json::parse(resp)["status"].get<std::string>();
        if (status == "done") { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto [code, body] = SendRequest(drogon::Get, "/media/" + id + "/result/result.png");
    EXPECT_EQ(code, 200);
    EXPECT_EQ(body, "fake-image");
}

TEST_F(WebApiTest, InvalidRequestsRejected) {
    // Invalid JSON
    auto [c1, r1] = SendRequest(drogon::Post, "/api/tasks", "{not json");
    EXPECT_EQ(c1, 400);

    // Missing source_paths
    auto [c2, r2] = SendRequest(drogon::Post, "/api/tasks", R"({"target_paths":["t.jpg"]})");
    EXPECT_EQ(c2, 400);

    // Unknown task detail
    auto [c3, r3] = SendRequest(drogon::Get, "/api/tasks/no_such_id");
    EXPECT_EQ(c3, 404);

    // Cancel unknown task -> ok:false
    auto [c4, r4] = SendRequest(drogon::Post, "/api/tasks/no_such_id/cancel");
    EXPECT_EQ(c4, 200);
    EXPECT_EQ(json::parse(r4)["ok"], false);

    // Media traversal blocked
    auto [c5, r5] = SendRequest(drogon::Get, "/media/no_such_id/result/..%2F..%2Fetc%2Fpasswd");
    EXPECT_EQ(c5, 404);
}

TEST_F(WebApiTest, CancelQueuedTask) {
    auto created = SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                              + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();
    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks/" + id + "/cancel");
    EXPECT_EQ(code, 200);
    EXPECT_EQ(json::parse(resp)["ok"], true);
    // Status ends cancelled or done (race: fake may finish first)
    auto [gcode, gresp] = SendRequest(drogon::Get, "/api/tasks/" + id);
    auto status = json::parse(gresp)["status"].get<std::string>();
    EXPECT_TRUE(status == "cancelled" || status == "done");
}
TEST_F(WebApiTest, UploadStoresFile) {
    // Upload with a binary body via raw HTTP helper (drogon client sets content type)
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/upload");
    req->addHeader("X-File-Name", "test_upload.jpg");
    req->setBody("fake-image-bytes");
    std::pair<int, std::string> out;
    std::promise<void> done;
    client->sendRequest(req, [&](drogon::ReqResult, const drogon::HttpResponsePtr& resp) {
        if (resp) {
            out.first = resp->getStatusCode();
            out.second = std::string(resp->getBody());
        }
        done.set_value();
    });
    done.get_future().wait();
    EXPECT_EQ(out.first, 201);
    auto body = json::parse(out.second);
    EXPECT_EQ(body["name"], "test_upload.jpg");
    EXPECT_EQ(body["size"], 16);
    auto path = std::filesystem::path(body["path"].get<std::string>());
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

TEST_F(WebApiTest, UploadRejectsBadFileName) {
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/upload");
    req->addHeader("X-File-Name", "../evil.jpg");
    req->setBody("x");
    std::pair<int, std::string> out;
    std::promise<void> done;
    client->sendRequest(req, [&](drogon::ReqResult, const drogon::HttpResponsePtr& resp) {
        if (resp) {
            out.first = resp->getStatusCode();
            out.second = std::string(resp->getBody());
        }
        done.set_value();
    });
    done.get_future().wait();
    EXPECT_EQ(out.first, 400);
}

TEST_F(WebApiTest, PriorityEndpointWorks) {
    // Block the executor so the first task stays running and later tasks queue
    g_executor->block.store(true);
    auto first = SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                            + kOutputDir + R"(","processors":["face_swapper"]})");
    // Wait until first task is running so the next one queues
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!g_tasks->is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_TRUE(g_tasks->is_running());

    auto created = SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                              + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();

    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks/" + id + "/priority",
                                    R"({"priority": 7})");
    EXPECT_EQ(code, 200);
    auto body = json::parse(resp);
    ASSERT_TRUE(body.contains("ok"));
    EXPECT_EQ(body["ok"], true);
    EXPECT_EQ(body["priority"], 7);

    // List reflects priority
    auto [lcode, lresp] = SendRequest(drogon::Get, "/api/tasks");
    auto list = json::parse(lresp);
    for (const auto& t : list) {
        if (t["id"] == id) { EXPECT_EQ(t["priority"], 7); }
    }

    // Bad payloads
    auto [b1, r1] = SendRequest(drogon::Post, "/api/tasks/" + id + "/priority", R"({"priority":"x"})");
    EXPECT_EQ(b1, 400);
    auto [b2, r2] = SendRequest(drogon::Post, "/api/tasks/no_such/priority", R"({"priority":1})");
    EXPECT_EQ(b2, 404);

    // Unblock and cancel so teardown is clean
    g_executor->block.store(false);
    g_tasks->cancel(first["id"].get<std::string>());
    g_tasks->cancel(id);
}

