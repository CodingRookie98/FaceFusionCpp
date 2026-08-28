#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

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

// ── Minimal raw-socket WS client (server frames are unmasked text) ──────

#ifdef _WIN32
using SocketHandle = SOCKET;
#define FFC_INVALID_SOCKET INVALID_SOCKET
#else
using SocketHandle = int;
#define FFC_INVALID_SOCKET (-1)
#endif

// 长跑集成负载下 WS 消息可能延迟（曾致 2s/3s 超时随机 flaky），统一放宽
static constexpr int kWsReadTimeoutMs = 10000;

void CloseSocket(SocketHandle fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    ::close(fd);
#endif
}

/// Connect + perform WS handshake; returns socket or FFC_INVALID_SOCKET.
SocketHandle WsConnect(const std::string& path) {
    SocketHandle fd = FFC_INVALID_SOCKET;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { return FFC_INVALID_SOCKET; }
#endif
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == FFC_INVALID_SOCKET) { return FFC_INVALID_SOCKET; }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kTestPort);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        CloseSocket(fd);
        return FFC_INVALID_SOCKET;
    }
    std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // fixed test key
    std::string req = "GET " + path
                    + " HTTP/1.1\r\n"
                      "Host: 127.0.0.1:"
                    + std::to_string(kTestPort)
                    + "\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Key: "
                    + key
                    + "\r\n"
                      "Sec-WebSocket-Version: 13\r\n\r\n";
    send(fd, req.data(), static_cast<int>(req.size()), 0);
    char buf[512];
    int n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        CloseSocket(fd);
        return FFC_INVALID_SOCKET;
    }
    buf[n] = '\0';
    if (std::string(buf).find("101") == std::string::npos) {
        CloseSocket(fd);
        return FFC_INVALID_SOCKET;
    }
    return fd;
}

/// Read one text frame (server->client frames are unmasked).
bool WsReadText(SocketHandle fd, std::string& out, int timeout_ms) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        unsigned char hdr[2];
        int n = recv(fd, hdr, 2, 0);
        if (n == 2) {
            std::size_t len = hdr[1] & 0x7F;
            if (len == 126) {
                unsigned char ext[2];
                if (recv(fd, ext, 2, 0) != 2) { return false; }
                len = (ext[0] << 8) | ext[1];
            }
            std::string payload(len, '\0');
            std::size_t got = 0;
            while (got < len) {
                int r = recv(fd, payload.data() + got, static_cast<int>(len - got), 0);
                if (r <= 0) { break; }
                got += static_cast<std::size_t>(r);
            }
            if (got == len) {
                out = payload;
                return true;
            }
            return false;
        }
        if (n < 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }
    return false;
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
    auto [code, resp] = SendRequest(
        drogon::Post, "/api/tasks",
        R"({"source_paths":["s.jpg"],"target_paths":["t.jpg"],"output_path":"ws_test_out"})");
    ASSERT_EQ(code, 201);
    auto id = json::parse(resp)["id"].get<std::string>();

    auto fd = WsConnect("/ws/tasks/" + id + "/progress");
    ASSERT_NE(fd, FFC_INVALID_SOCKET) << "WS handshake failed";

    int progress_count = 0;
    bool saw_done = false;
    std::string msg;
    while (WsReadText(fd, msg, kWsReadTimeoutMs)) {
        auto j = json::parse(msg);
        if (j["type"] == "progress") {
            ++progress_count;
        } else if (j["type"] == "status" && j["status"] == "done") {
            saw_done = true;
            break;
        } else if (j["type"] == "status" && j["status"] == "failed") {
            break;
        }
    }
    CloseSocket(fd);
    EXPECT_GE(progress_count, 1);
    EXPECT_TRUE(saw_done);
}

TEST_F(WebWsTest, UnknownTaskGetsErrorMessage) {
    auto fd = WsConnect("/ws/tasks/nonexistent_ws_task/progress");
    ASSERT_NE(fd, FFC_INVALID_SOCKET) << "WS handshake failed";
    // 冷启动防抖：连接建立回调的 send 已经事件循环延迟投递（产品侧修复），轮询保底
    std::string msg;
    bool got_frame = false;
    for (int attempt = 0; attempt < 5 && !got_frame; ++attempt) {
        got_frame = WsReadText(fd, msg, kWsReadTimeoutMs / 5);
    }
    ASSERT_TRUE(got_frame) << "no frame received";
    CloseSocket(fd);
    auto j = json::parse(msg);
    EXPECT_EQ(j["type"], "error");
    EXPECT_EQ(j["message"], "task not found");
}
