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
    auto seed =
        static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::mt19937 gen(seed);
    return static_cast<uint16_t>(20000 + (gen() % 20000)); // 20000-39999
}
const uint16_t kTestPort = RandomTestPort();
const std::string kBaseUrl = "http://127.0.0.1:" + std::to_string(kTestPort);
const std::string kOutputDir = "web_api_test_output";

/// Fake executor: creates an output file (simulates a finished task)
class FakeExecutor : public ITaskExecutor {
public:
    int run(const config::TaskConfig& config,
            const services::pipeline::ProgressCallback& cb) override {
        cancelled.store(false);
        if (block.load()) {
            while (!cancelled.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
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
        run_server(
            {.host = "127.0.0.1", .port = kTestPort, .web_root = ""},
            {.tasks = g_tasks, .app_config = nullptr, .detect_faces = [](const std::string& path) {
                 if (path.find("test_face_detect.jpg") != std::string::npos) {
                     DetectedFaceInfo face;
                     face.index = 0;
                     face.box = {10.0F, 20.0F, 80.0F, 90.0F};
                     face.score = 0.95F;
                     face.gender = "female";
                     face.age_range = {20, 30};
                     face.kps = {{30.0F, 40.0F}, {70.0F, 40.0F}};
                     return std::vector<DetectedFaceInfo>{face};
                 }
                 return std::vector<DetectedFaceInfo>{};
             }});
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
    auto created =
        SubmitTask(R"({"source_paths":["src.jpg"],"target_paths":["tgt.jpg"],"output_path":")"
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
    auto created =
        SubmitTask(R"({"source_paths":["src.jpg"],"target_paths":["tgt.jpg"],"output_path":")"
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
    auto created =
        SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
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

TEST_F(WebApiTest, UploadHandlesNonAsciiUrlEncodedFileName) {
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/api/upload");
    req->setMethod(drogon::Post);
    // URL-encoded UTF-8 filename for "我的头像_测试.jpg"
    std::string encoded_name = "%E6%88%91%E7%9A%84%E5%A4%B4%E5%83%8F_%E6%B5%8B%E8%AF%95.jpg";
    req->addHeader("X-File-Name", encoded_name);
    req->setBody("binary_photo_data_123");
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
    auto res = json::parse(out.second);
    EXPECT_EQ(res["name"], "我的头像_测试.jpg");
    EXPECT_EQ(res["size"], 21);
    EXPECT_TRUE(std::filesystem::exists(res["path"].get<std::string>()));
    std::filesystem::remove(res["path"].get<std::string>());
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

    auto created =
        SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                   + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();

    auto [code, resp] =
        SendRequest(drogon::Post, "/api/tasks/" + id + "/priority", R"({"priority": 7})");
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
    auto [b1, r1] =
        SendRequest(drogon::Post, "/api/tasks/" + id + "/priority", R"({"priority":"x"})");
    EXPECT_EQ(b1, 400);
    auto [b2, r2] = SendRequest(drogon::Post, "/api/tasks/no_such/priority", R"({"priority":1})");
    EXPECT_EQ(b2, 404);

    // Unblock and cancel so teardown is clean
    g_executor->block.store(false);
    g_tasks->cancel(first["id"].get<std::string>());
    g_tasks->cancel(id);
}

TEST_F(WebApiTest, DetectFacesPostAndGet) {
    // Create a temporary dummy file to detect
    std::ofstream("test_face_detect.jpg") << "fake_image_content";

    // 1. POST /api/faces with JSON body
    auto [code1, resp1] =
        SendRequest(drogon::Post, "/api/faces", R"({"image_path": "test_face_detect.jpg"})");
    EXPECT_EQ(code1, 200);
    auto body1 = json::parse(resp1);
    EXPECT_EQ(body1["image"], "test_face_detect.jpg");
    ASSERT_TRUE(body1["faces"].is_array());
    ASSERT_EQ(body1["faces"].size(), 1);
    EXPECT_EQ(body1["faces"][0]["index"], 0);
    EXPECT_FLOAT_EQ(body1["faces"][0]["box"]["x"], 10.0F);
    EXPECT_FLOAT_EQ(body1["faces"][0]["box"]["y"], 20.0F);
    EXPECT_FLOAT_EQ(body1["faces"][0]["box"]["width"], 80.0F);
    EXPECT_FLOAT_EQ(body1["faces"][0]["box"]["height"], 90.0F);
    EXPECT_FLOAT_EQ(body1["faces"][0]["score"], 0.95F);
    EXPECT_EQ(body1["faces"][0]["gender"], "female");
    EXPECT_EQ(body1["faces"][0]["age_range"][0], 20);
    EXPECT_EQ(body1["faces"][0]["age_range"][1], 30);
    ASSERT_EQ(body1["faces"][0]["kps"].size(), 2);
    EXPECT_FLOAT_EQ(body1["faces"][0]["kps"][0]["x"], 30.0F);
    EXPECT_FLOAT_EQ(body1["faces"][0]["kps"][0]["y"], 40.0F);

    // 2. GET /api/faces?image=test_face_detect.jpg
    auto [code2, resp2] = SendRequest(drogon::Get, "/api/faces?image=test_face_detect.jpg");
    EXPECT_EQ(code2, 200);
    auto body2 = json::parse(resp2);
    EXPECT_EQ(body2["image"], "test_face_detect.jpg");
    ASSERT_EQ(body2["faces"].size(), 1);

    // 3. Error cases
    // Bad request: missing image parameter
    auto [code3, _3] = SendRequest(drogon::Post, "/api/faces", R"({})");
    EXPECT_EQ(code3, 400);

    // Bad request: invalid JSON
    auto [code4, _4] = SendRequest(drogon::Post, "/api/faces", "{invalid");
    EXPECT_EQ(code4, 400);

    // Bad request: path traversal
    auto [code5, _5] =
        SendRequest(drogon::Post, "/api/faces", R"({"image_path": "../outside.jpg"})");
    EXPECT_EQ(code5, 400);

    // Not found: non-existent file
    auto [code6, _6] =
        SendRequest(drogon::Post, "/api/faces", R"({"image_path": "non_existent.jpg"})");
    EXPECT_EQ(code6, 404);

    std::filesystem::remove("test_face_detect.jpg");
}

TEST_F(WebApiTest, ProcessorsEndpointReturnsMeta) {
    auto [code, resp] = SendRequest(drogon::Get, "/api/processors");
    EXPECT_EQ(code, 200);
    auto body = json::parse(resp);
    ASSERT_TRUE(body.is_array());
    ASSERT_FALSE(body.empty());

    bool found_swapper = false;
    for (const auto& proc : body) {
        ASSERT_TRUE(proc.contains("name"));
        ASSERT_TRUE(proc.contains("params"));
        if (proc["name"] == "face_swapper") {
            found_swapper = true;
            EXPECT_TRUE(proc["params"].is_array());
        }
    }
    EXPECT_TRUE(found_swapper);
}

TEST_F(WebApiTest, TaskProgressEndpointWorks) {
    auto created =
        SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                   + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();

    auto [code, resp] = SendRequest(drogon::Get, "/api/tasks/" + id + "/progress");
    EXPECT_EQ(code, 200);
    auto body = json::parse(resp);
    EXPECT_EQ(body["id"], id);
    ASSERT_TRUE(body.contains("status"));
    ASSERT_TRUE(body.contains("progress"));
    EXPECT_TRUE(body["progress"].contains("current_frame"));
    EXPECT_TRUE(body["progress"].contains("total_frames"));
    EXPECT_TRUE(body["progress"].contains("fps"));

    // 404 on invalid task id
    auto [code404, _] = SendRequest(drogon::Get, "/api/tasks/non_existent_id/progress");
    EXPECT_EQ(code404, 404);
}

TEST_F(WebApiTest, MediaEndpointSupportsRangeAndVideo) {
    auto created =
        SubmitTask(R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":")"
                   + kOutputDir + R"(","processors":["face_swapper"]})");
    std::string id = created["id"].get<std::string>();

    // 输出目录按 task_id 物理隔离；将测试文件放入任务隔离目录
    std::string isolated_dir = (std::filesystem::path(kOutputDir) / id).string();
    std::filesystem::create_directories(isolated_dir);
    std::string test_file = (std::filesystem::path(isolated_dir) / "sample_video.mp4").string();
    {
        std::ofstream out(test_file, std::ios::binary);
        out << "0123456789ABCDEF"; // 16 bytes
    }

    // Test standard GET /media/{id}/result/sample_video.mp4
    auto [code1, resp1] = SendRequest(drogon::Get, "/media/" + id + "/result/sample_video.mp4");
    EXPECT_EQ(code1, 200);
    EXPECT_EQ(resp1.size(), 16);

    // Test HTTP Range GET (Range: bytes=0-3)
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/media/" + id + "/result/sample_video.mp4");
    req->addHeader("Range", "bytes=0-3");
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
    EXPECT_EQ(out.first, 206); // 206 Partial Content
    EXPECT_EQ(out.second, "0123");

    std::filesystem::remove(test_file);
}

TEST_F(WebApiTest, SubmitTaskWithPipelineSteps) {
    std::string payload = R"({
        "source_paths": ["s1.jpg", "s2.jpg"],
        "target_paths": ["t.jpg"],
        "output_path": ")"
                        + kOutputDir + R"(",
        "pipeline_steps": [
            {
                "step": "face_swapper",
                "name": "主角换脸",
                "enabled": true,
                "params": {
                    "model": "inswapper_128",
                    "face_selector_mode": "reference",
                    "reference_face_path": "s1.jpg"
                }
            },
            {
                "step": "face_swapper",
                "name": "配角换脸",
                "enabled": true,
                "params": {
                    "model": "inswapper_128",
                    "face_selector_mode": "reference",
                    "reference_face_path": "s2.jpg"
                }
            },
            {
                "step": "face_enhancer",
                "name": "高清细节增强",
                "enabled": true,
                "params": {
                    "model": "codeformer",
                    "blend_factor": 0.85,
                    "face_selector_mode": "many"
                }
            }
        ]
    })";

    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks", payload);
    ASSERT_EQ(code, 201);
    auto created = json::parse(resp);
    EXPECT_TRUE(created.contains("id"));
    EXPECT_EQ(created["status"], "queued");

    std::string id = created["id"].get<std::string>();
    auto [dcode, dresp] = SendRequest(drogon::Get, "/api/tasks/" + id);
    ASSERT_EQ(dcode, 200);
}

TEST_F(WebApiTest, PipelineStepsValidation) {
    // 1. pipeline_steps not array
    auto [c1, r1] = SendRequest(drogon::Post, "/api/tasks", R"({
        "source_paths": ["s.jpg"],
        "target_paths": ["t.jpg"],
        "pipeline_steps": "not_an_array"
    })");
    EXPECT_EQ(c1, 400);

    // 2. pipeline_steps entry missing 'step'
    auto [c2, r2] = SendRequest(drogon::Post, "/api/tasks", R"({
        "source_paths": ["s.jpg"],
        "target_paths": ["t.jpg"],
        "pipeline_steps": [{"name": "invalid_step"}]
    })");
    EXPECT_EQ(c2, 400);

    // 3. pipeline_steps unknown processor step
    auto [c3, r3] = SendRequest(drogon::Post, "/api/tasks", R"({
        "source_paths": ["s.jpg"],
        "target_paths": ["t.jpg"],
        "pipeline_steps": [{"step": "non_existent_processor"}]
    })");
    EXPECT_EQ(c3, 400);

    // 4. pipeline_steps with invalid selector mode
    auto [c4, r4] = SendRequest(drogon::Post, "/api/tasks", R"({
        "source_paths": ["s.jpg"],
        "target_paths": ["t.jpg"],
        "pipeline_steps": [{
            "step": "face_swapper",
            "params": {"face_selector_mode": "illegal_mode"}
        }]
    })");
    EXPECT_EQ(c4, 400);
}

TEST_F(WebApiTest, SubmitTaskWithAllFourProcessors) {
    std::string payload = R"({
        "source_paths": ["s.jpg"],
        "target_paths": ["t.jpg"],
        "output_path": ")"
                        + kOutputDir + R"(",
        "pipeline_steps": [
            {
                "step": "face_swapper",
                "name": "电影级换脸",
                "enabled": true,
                "params": {
                    "model": "inswapper_128",
                    "face_selector_mode": "many"
                }
            },
            {
                "step": "face_enhancer",
                "name": "GFPGAN增强",
                "enabled": true,
                "params": {
                    "model": "gfpgan_1.4",
                    "blend_factor": 0.9,
                    "face_selector_mode": "many"
                }
            },
            {
                "step": "expression_restorer",
                "name": "微表情修复",
                "enabled": true,
                "params": {
                    "model": "live_portrait",
                    "restore_factor": 0.7,
                    "face_selector_mode": "many"
                }
            },
            {
                "step": "frame_enhancer",
                "name": "超分放大",
                "enabled": true,
                "params": {
                    "model": "real_esrgan_x4",
                    "enhance_factor": 1.0
                }
            }
        ]
    })";

    auto [code, resp] = SendRequest(drogon::Post, "/api/tasks", payload);
    ASSERT_EQ(code, 201);
    auto created = json::parse(resp);
    EXPECT_TRUE(created.contains("id"));
    EXPECT_EQ(created["status"], "queued");

    std::string id = created["id"].get<std::string>();
    auto [dcode, dresp] = SendRequest(drogon::Get, "/api/tasks/" + id);
    ASSERT_EQ(dcode, 200);
}

TEST_F(WebApiTest, FaceDetectionErrorHandling) {
    // Non-existent image path -> 404 Not Found
    auto [code, resp] = SendRequest(drogon::Post, "/api/faces", R"({
        "image_path": "non_existent_file_path_123456.jpg"
    })");
    EXPECT_EQ(code, 404);

    // Empty body -> 400 Bad Request
    auto [code2, resp2] = SendRequest(drogon::Post, "/api/faces", "");
    EXPECT_EQ(code2, 400);

    // GET without query parameter -> 400 Bad Request
    auto [code3, resp3] = SendRequest(drogon::Get, "/api/faces");
    EXPECT_EQ(code3, 400);
}

TEST_F(WebApiTest, PreviewEndpointWorks) {
    // 1. Create a dummy test file
    std::filesystem::create_directories("./temp");
    std::string test_file = "./temp/preview_test_sample.txt";
    {
        std::ofstream out(test_file);
        out << "preview_sample_content";
    }

    // 2. Request preview
    auto [code, resp] = SendRequest(drogon::Get, "/api/preview?path=" + test_file);
    EXPECT_EQ(code, 200);
    EXPECT_EQ(resp, "preview_sample_content");

    // 3. Traversal rejection
    auto [c2, r2] = SendRequest(drogon::Get, "/api/preview?path=../secret.txt");
    EXPECT_EQ(c2, 400);

    // 4. Missing path param
    auto [c3, r3] = SendRequest(drogon::Get, "/api/preview");
    EXPECT_EQ(c3, 400);

    // 5. File not found
    auto [c4, r4] = SendRequest(drogon::Get, "/api/preview?path=./temp/not_existing_file.xyz");
    EXPECT_EQ(c4, 404);

    std::filesystem::remove(test_file);
}

TEST_F(WebApiTest, LargeUploadSucceedsAbove1MB) {
    auto client = drogon::HttpClient::newHttpClient(kBaseUrl);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setPath("/api/upload");
    req->setMethod(drogon::Post);
    req->addHeader("X-File-Name", "large_video_target.mp4");
    // 2MB payload (> default Drogon 1MB limit)
    std::string large_body(2 * 1024 * 1024, 'A');
    req->setBody(std::move(large_body));

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
    auto res = json::parse(out.second);
    EXPECT_EQ(res["size"], 2 * 1024 * 1024);
    EXPECT_TRUE(std::filesystem::exists(res["path"].get<std::string>()));
    std::filesystem::remove(res["path"].get<std::string>());
}

TEST_F(WebApiTest, SubmitTaskGeneratesValidTaskConfig) {
    std::filesystem::create_directories("./temp");
    std::string src_file = "./temp/test_source_valid.jpg";
    std::string tgt_file = "./temp/test_target_valid.jpg";
    {
        std::ofstream(src_file) << "fake_src";
        std::ofstream(tgt_file) << "fake_tgt";
    }

    json payload = {
        {"source_paths", {src_file}},
        {"target_paths", {tgt_file}},
        {"pipeline_steps",
         {{{"step", "face_swapper"},
           {"name", "主角换脸"},
           {"enabled", true},
           {"params", {{"model", "inswapper_128_fp16"}, {"face_selector_mode", "reference"}}}}}}};

    auto created = SubmitTask(payload.dump());
    std::string id = created["id"].get<std::string>();

    auto [code, resp] = SendRequest(drogon::Get, "/api/tasks/" + id);
    EXPECT_EQ(code, 200);
    auto detail = json::parse(resp);
    EXPECT_EQ(detail["id"], id);
    // 任务输出目录按 task_id 物理隔离 (./output/{task_id})
    auto output_path = detail["output_path"].get<std::string>();
    EXPECT_NE(output_path.find("./output/"), std::string::npos);
    EXPECT_NE(output_path.find(id), std::string::npos);

    std::filesystem::remove(src_file);
    std::filesystem::remove(tgt_file);
}
