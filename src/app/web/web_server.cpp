module;
#include <chrono>
#include <string>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <sstream>

#include <drogon/drogon.h>
#include <drogon/DrClassMap.h>
#include <drogon/WebSocketController.h>
#include <drogon/WebSocketConnection.h>
#include <drogon/utils/Utilities.h>
#include <nlohmann/json.hpp>

namespace {
constexpr std::size_t kMaxUploadBytes = 1024ULL * 1024 * 1024; // 1GB
}

module app.web.server;

import app.version;
import app.web.task_manager;
import app.web.task_types;
import config.task;
import config.merger;
import processor.param_registry;

namespace app::web {

namespace {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────
// JSON serialization helpers
// ─────────────────────────────────────────────────────────────────────────

std::string status_to_string(TaskStatus status) {
    switch (status) {
    case TaskStatus::Queued: return "queued";
    case TaskStatus::Running: return "running";
    case TaskStatus::Done: return "done";
    case TaskStatus::Failed: return "failed";
    case TaskStatus::Cancelled: return "cancelled";
    }
    return "unknown";
}

json progress_to_json(const TaskProgress& p) {
    return {{"current_frame", p.current_frame}, {"total_frames", p.total_frames}, {"fps", p.fps}};
}

json task_summary_to_json(const TaskSummary& s) {
    return {{"id", s.id},
            {"status", status_to_string(s.status)},
            {"progress", progress_to_json(s.progress)},
            {"error_message", s.error_message},
            {"media_count", s.media_count},
            {"priority", s.priority},
            {"queue_position", s.queue_position}};
}

json task_entry_to_json(const TaskEntry& e) {
    // Media URLs use whitelist indices: /media/{task_id}/source/{idx}
    json media = json::object();
    json sources = json::array();
    for (std::size_t i = 0; i < e.config.io.source_paths.size(); ++i) {
        sources.push_back("/media/" + e.id + "/source/" + std::to_string(i));
    }
    media["source"] = sources;
    json targets = json::array();
    for (std::size_t i = 0; i < e.config.io.target_paths.size(); ++i) {
        targets.push_back("/media/" + e.id + "/target/" + std::to_string(i));
    }
    media["target"] = targets;

    json results = json::array();
    for (const auto& f : e.result_files) {
        results.push_back({{"name", f}, {"url", "/media/" + e.id + "/result/" + f}});
    }

    return {{"id", e.id},
            {"status", status_to_string(e.status)},
            {"progress", progress_to_json(e.progress)},
            {"error_message", e.error_message},
            {"output_path", e.config.io.output.path},
            {"media", media},
            {"results", results},
            {"priority", e.priority}};
}

json detected_face_to_json(const DetectedFaceInfo& f) {
    json kps = json::array();
    for (const auto& pt : f.kps) { kps.push_back({{"x", pt.x}, {"y", pt.y}}); }
    json j = {
        {"index", f.index},
        {"box", {{"x", f.box.x}, {"y", f.box.y}, {"width", f.box.width}, {"height", f.box.height}}},
        {"score", f.score},
        {"gender", f.gender},
        {"age_range", {f.age_range.first, f.age_range.second}},
        {"kps", kps}};
    return j;
}

// ─────────────────────────────────────────────────────────────────────────
// TaskConfig parsing from POST body
// ─────────────────────────────────────────────────────────────────────────

bool parse_task_config(const json& body, config::TaskConfig& out, std::string& err) {
    out.config_version = "1.0";
    out.task_info.id = "web_task";

    // io.source_paths
    if (!body.contains("source_paths") || !body["source_paths"].is_array()
        || body["source_paths"].empty()) {
        err = "source_paths (non-empty array) is required";
        return false;
    }
    for (const auto& p : body["source_paths"]) {
        if (!p.is_string()) {
            err = "source_paths entries must be strings";
            return false;
        }
        out.io.source_paths.push_back(p.get<std::string>());
    }

    // io.target_paths
    if (!body.contains("target_paths") || !body["target_paths"].is_array()
        || body["target_paths"].empty()) {
        err = "target_paths (non-empty array) is required";
        return false;
    }
    for (const auto& p : body["target_paths"]) {
        if (!p.is_string()) {
            err = "target_paths entries must be strings";
            return false;
        }
        out.io.target_paths.push_back(p.get<std::string>());
    }

    // io.output.path
    if (body.contains("output_path")) {
        if (!body["output_path"].is_string()) {
            err = "output_path must be a string";
            return false;
        }
        out.io.output.path = body["output_path"].get<std::string>();
    }

    // 1. New structured pipeline_steps support
    if (body.contains("pipeline_steps")) {
        if (!body["pipeline_steps"].is_array()) {
            err = "pipeline_steps must be an array";
            return false;
        }
        for (const auto& item : body["pipeline_steps"]) {
            if (!item.is_object()) {
                err = "pipeline_steps entry must be an object";
                return false;
            }
            if (!item.contains("step") || !item["step"].is_string()) {
                err = "pipeline_steps entry must contain a string 'step'";
                return false;
            }
            config::PipelineStep step;
            step.step = item["step"].get<std::string>();
            if (!domain::processor::ProcessorParamRegistry::instance().find(step.step)) {
                err = "Unknown processor step: " + step.step;
                return false;
            }
            if (item.contains("name") && item["name"].is_string()) {
                step.name = item["name"].get<std::string>();
            } else {
                step.name = step.step;
            }
            if (item.contains("enabled") && item["enabled"].is_boolean()) {
                step.enabled = item["enabled"].get<bool>();
            } else {
                step.enabled = true;
            }
            if (item.contains("params") && item["params"].is_object()) {
                for (auto pit = item["params"].begin(); pit != item["params"].end(); ++pit) {
                    step.cli_params[pit.key()] = pit.value().is_string() ?
                                                     pit.value().get<std::string>() :
                                                     pit.value().dump();
                }
            }
            auto apply_r = config::ApplyCliParamsToStep(step);
            if (!apply_r) {
                err = apply_r.error().message;
                return false;
            }
            out.pipeline.push_back(std::move(step));
        }
        if (out.pipeline.empty()) {
            err = "pipeline_steps must not be empty";
            return false;
        }
        return true;
    }

    // 2. Legacy processors + processor_params fallback
    std::vector<std::string> processors;
    if (body.contains("processors")) {
        if (!body["processors"].is_array()) {
            err = "processors must be an array";
            return false;
        }
        for (const auto& p : body["processors"]) {
            if (!p.is_string()) {
                err = "processors entries must be strings";
                return false;
            }
            processors.push_back(p.get<std::string>());
        }
    }
    if (processors.empty()) { processors = {"face_swapper"}; }

    std::map<std::string, std::map<std::string, std::string>> params;
    if (body.contains("processor_params")) {
        const auto& pp = body["processor_params"];
        if (!pp.is_object()) {
            err = "processor_params must be an object";
            return false;
        }
        for (auto it = pp.begin(); it != pp.end(); ++it) {
            if (!it.value().is_object()) { continue; }
            for (auto pit = it.value().begin(); pit != it.value().end(); ++pit) {
                params[it.key()][pit.key()] =
                    pit.value().is_string() ? pit.value().get<std::string>() : pit.value().dump();
            }
        }
    }

    for (const auto& proc : processors) {
        config::PipelineStep step;
        step.step = proc;
        step.enabled = true;
        auto it = params.find(proc);
        if (it != params.end()) {
            for (const auto& [k, v] : it->second) { step.cli_params[k] = v; }
        }
        auto apply_r = config::ApplyCliParamsToStep(step);
        if (!apply_r) {
            err = apply_r.error().message;
            return false;
        }
        out.pipeline.push_back(std::move(step));
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────
// Path parameter helper
// NOTE: drogon 1.9.x getParameter() only resolves query-string parameters;
// path parameters must be read positionally via getRoutingParameters().
// ─────────────────────────────────────────────────────────────────────────

std::string path_param(const drogon::HttpRequestPtr& req, std::size_t index) {
    const auto& params = req->getRoutingParameters();
    return index < params.size() ? params[index] : std::string();
}

// ─────────────────────────────────────────────────────────────────────────
// /media/ whitelist file serving
// ─────────────────────────────────────────────────────────────────────────

bool is_path_within(const std::filesystem::path& file, const std::filesystem::path& dir) {
    std::error_code ec;
    auto file_canon = std::filesystem::weakly_canonical(file, ec);
    auto dir_canon = std::filesystem::weakly_canonical(dir, ec);
    if (ec) { return false; }
    auto rel = file_canon.lexically_relative(dir_canon);
    return !rel.empty() && rel.native()[0] != '.' && rel.native().find("..") == std::string::npos;
}

void serve_file(const std::filesystem::path& path, const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
        auto resp = drogon::HttpResponse::newNotFoundResponse();
        cb(resp);
        return;
    }

    std::string range_header = req ? req->getHeader("Range") : "";
    if (range_header.empty() && req) { range_header = req->getHeader("range"); }
    if (!range_header.empty() && range_header.starts_with("bytes=")) {
        std::string spec = range_header.substr(6);
        auto dash_pos = spec.find('-');
        if (dash_pos != std::string::npos) {
            std::size_t file_size = std::filesystem::file_size(path);
            std::string start_str = spec.substr(0, dash_pos);
            std::string end_str = spec.substr(dash_pos + 1);
            std::size_t start = 0;
            std::size_t end = file_size > 0 ? file_size - 1 : 0;
            if (!start_str.empty()) { start = std::stoull(start_str); }
            if (!end_str.empty()) { end = std::stoull(end_str); }
            if (start <= end && start < file_size) {
                std::size_t length = std::min(end + 1, file_size) - start;
                auto resp = drogon::HttpResponse::newFileResponse(
                    path.string(), start, length, true, "", drogon::CT_NONE, "", req);
                resp->addHeader("Accept-Ranges", "bytes");
                cb(resp);
                return;
            }
        }
    }
    auto resp = drogon::HttpResponse::newFileResponse(path.string(), "", drogon::CT_NONE, "", req);
    resp->addHeader("Accept-Ranges", "bytes");
    cb(resp);
}

// ─────────────────────────────────────────────────────────────────────────
// WebSocket progress push controller
// Endpoint: /ws/tasks/{task_id}/progress
// ─────────────────────────────────────────────────────────────────────────

class ProgressWSController : public drogon::WebSocketController<ProgressWSController> {
public:
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/tasks/{task_id}/progress");
    WS_PATH_LIST_END

    static std::shared_ptr<TaskManager> tasks;
    static std::mutex subs_mutex;
    static std::map<std::string, std::vector<drogon::WebSocketConnectionPtr>> subscriptions;

    static void broadcast_progress(const std::string& task_id, const TaskProgress& progress) {
        json msg = {{"type", "progress"},
                    {"frame", progress.current_frame},
                    {"total", progress.total_frames},
                    {"fps", progress.fps}};
        broadcast(task_id, msg.dump());
    }

    static void broadcast_status(const std::string& task_id, TaskStatus status,
                                 const std::string& error) {
        json msg = {{"type", "status"}, {"status", status_to_string(status)}};
        if (!error.empty()) { msg["message"] = error; }
        broadcast(task_id, msg.dump());
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr&, std::string&&,
                          const drogon::WebSocketMessageType&) override {
        // Client messages are ignored in M2 (ping/pong is handled by drogon)
    }

    /// Extract task id from "/ws/tasks/<id>/progress".
    /// (drogon regex WS routes do not populate getRoutingParameters())
    static std::string task_id_from_path(const std::string& path) {
        const std::string prefix = "/ws/tasks/";
        const std::string suffix = "/progress";
        if (path.rfind(prefix, 0) != 0 || path.size() <= prefix.size() + suffix.size()) {
            return {};
        }
        if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) { return {}; }
        return path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
    }

    void handleNewConnection(const drogon::HttpRequestPtr& req,
                             const drogon::WebSocketConnectionPtr& wsConn) override {
        auto task_id = task_id_from_path(req->path());
        {
            std::lock_guard lock(subs_mutex);
            subscriptions[task_id].push_back(wsConn);
        }
        auto entry = tasks->get(task_id);
        if (!entry) {
            wsConn->send(json{{"type", "error"}, {"message", "task not found"}}.dump());
            return;
        }
        wsConn->send(json{{"type", "status"}, {"status", status_to_string(entry->status)}}.dump());
        if (entry->status == TaskStatus::Running || entry->status == TaskStatus::Queued) {
            broadcast_progress(task_id, entry->progress);
        }
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConn) override {
        std::lock_guard lock(subs_mutex);
        for (auto& [tid, conns] : subscriptions) { std::erase(conns, wsConn); }
    }

private:
    static void broadcast(const std::string& task_id, const std::string& msg) {
        std::lock_guard lock(subs_mutex);
        auto it = subscriptions.find(task_id);
        if (it == subscriptions.end()) { return; }
        for (const auto& conn : it->second) {
            if (conn->connected()) { conn->send(msg); }
        }
    }
};

std::shared_ptr<TaskManager> ProgressWSController::tasks;
std::mutex ProgressWSController::subs_mutex;
std::map<std::string, std::vector<drogon::WebSocketConnectionPtr>>
    ProgressWSController::subscriptions;

// Force odr-use so the drogon auto-creation machinery instantiates
// DrObject::alloc_ (class registration) and the controller's pathRegistrator_
// (route registration) at static-init time. Without an instance the compiler
// never emits those template static members and the WS route stays unregistered.
static ProgressWSController g_ws_controller_registrar;

} // namespace

// ─────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────

std::string health_json() {
    return "{\"status\":\"ok\",\"version\":\"" + std::string(app::version::version()) + "\"}";
}

void run_server(const WebServerOptions& options, const WebServerDeps& deps) {
    auto& app = drogon::app();
    auto tasks = deps.tasks;
    const std::string temp_dir =
        deps.app_config != nullptr && !deps.app_config->temp_directory.empty() ?
            deps.app_config->temp_directory :
            "./temp";

    // Wire WebSocket progress push.
    // The module build skips static-init of template static members
    // (DrObject::alloc_ / registrator_), so we create the controller instance
    // explicitly and register it via DrClassMap before registering the route.
    auto ws_instance = std::make_shared<ProgressWSController>();
    drogon::DrClassMap::setSingleInstance(ws_instance);
    ProgressWSController::tasks = tasks;
    ProgressWSController::subscriptions.clear();
    // Note: registerWebSocketController() matches exact paths only; parameterized
    // routes must be registered via the regex API.
    app.registerWebSocketControllerRegex("/ws/tasks/([0-9a-zA-Z_]+)/progress",
                                         ProgressWSController::classTypeName());
    tasks->set_progress_listener([](const std::string& task_id, const TaskProgress& progress) {
        ProgressWSController::broadcast_progress(task_id, progress);
    });
    tasks->set_status_listener(
        [](const std::string& task_id, TaskStatus status, const std::string& error) {
            ProgressWSController::broadcast_status(task_id, status, error);
        });

    app.setClientMaxBodySize(kMaxUploadBytes);
    app.setClientMaxMemoryBodySize(64ULL * 1024 * 1024);

    app.addListener(options.host, options.port);

    // GET /api/preview?path=<encoded_path>
    app.registerHandler("/api/preview",
                        [](const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto path_param = req->getParameter("path");
                            if (path_param.empty()) { path_param = req->getParameter("file"); }
                            if (path_param.empty()) {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody(json{{"error", "path parameter is required"}}.dump());
                                cb(resp);
                                return;
                            }

                            auto decoded_path = drogon::utils::urlDecode(path_param);
                            if (decoded_path.find("..") != std::string::npos) {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody(json{{"error", "invalid path"}}.dump());
                                cb(resp);
                                return;
                            }

                            std::filesystem::path target_file(decoded_path);
                            if (!std::filesystem::exists(target_file)) {
                                if (target_file.is_relative()) {
                                    auto resolved = std::filesystem::current_path() / target_file;
                                    if (std::filesystem::exists(resolved)) {
                                        target_file = resolved;
                                    }
                                }
                            }

                            if (!std::filesystem::exists(target_file)
                                || std::filesystem::is_directory(target_file)) {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                                resp->setStatusCode(drogon::k404NotFound);
                                resp->setBody(json{{"error", "file not found"}}.dump());
                                cb(resp);
                                return;
                            }

                            serve_file(target_file, req, std::move(cb));
                        },
                        {drogon::Get});

    // GET /api/health
    app.registerHandler("/api/health",
                        [](const drogon::HttpRequestPtr&,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            resp->setBody(health_json());
                            cb(resp);
                        },
                        {drogon::Get});

    // GET /api/processors
    app.registerHandler(
        "/api/processors",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

            const auto& registry = domain::processor::ProcessorParamRegistry::instance();
            auto names = registry.all_processor_names();
            json arr = json::array();
            for (const auto& name : names) {
                const auto* meta = registry.find(name);
                if (!meta) continue;
                json proc = json::object();
                proc["name"] = meta->name;
                json params = json::array();
                for (const auto& p : meta->params) {
                    json pm = json::object();
                    pm["name"] = p.name;
                    std::string type_str = "string";
                    switch (p.type) {
                    case domain::processor::ParamType::String: type_str = "string"; break;
                    case domain::processor::ParamType::Int: type_str = "int"; break;
                    case domain::processor::ParamType::Float: type_str = "float"; break;
                    case domain::processor::ParamType::Bool: type_str = "bool"; break;
                    case domain::processor::ParamType::Path: type_str = "path"; break;
                    }
                    pm["type"] = type_str;
                    pm["description"] = p.description;
                    pm["allowed_values"] = p.allowed_values;
                    if (p.range) { pm["range"] = json::array({p.range->first, p.range->second}); }
                    params.push_back(pm);
                }
                proc["params"] = params;
                arr.push_back(proc);
            }
            resp->setBody(arr.dump());
            cb(resp);
        },
        {drogon::Get});

    // POST /api/tasks
    app.registerHandler("/api/tasks",
                        [tasks](const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            json body;
                            try {
                                body = json::parse(req->getBody());
                            } catch (const std::exception&) {
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody(json{{"error", "invalid JSON body"}}.dump());
                                cb(resp);
                                return;
                            }
                            config::TaskConfig task_config;
                            std::string err;
                            if (!parse_task_config(body, task_config, err)) {
                                resp->setStatusCode(drogon::k400BadRequest);
                                resp->setBody(json{{"error", err}}.dump());
                                cb(resp);
                                return;
                            }
                            auto id = tasks->submit(std::move(task_config));
                            resp->setStatusCode(drogon::k201Created);
                            resp->setBody(json{{"id", id}, {"status", "queued"}}.dump());
                            cb(resp);
                        },
                        {drogon::Post});

    // GET /api/tasks
    app.registerHandler("/api/tasks",
                        [tasks](const drogon::HttpRequestPtr&,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            json arr = json::array();
                            for (const auto& s : tasks->list()) {
                                arr.push_back(task_summary_to_json(s));
                            }
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            resp->setBody(arr.dump());
                            cb(resp);
                        },
                        {drogon::Get});

    // GET / POST /api/faces
    auto handle_faces = [deps](const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        std::string image_path;
        if (req->method() == drogon::Get) {
            image_path = req->getParameter("image");
            if (image_path.empty()) { image_path = req->getParameter("image_path"); }
        } else if (req->method() == drogon::Post) {
            json body;
            try {
                body = json::parse(req->getBody());
            } catch (const std::exception&) {
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody(json{{"error", "invalid JSON body"}}.dump());
                cb(resp);
                return;
            }
            if (body.contains("image_path") && body["image_path"].is_string()) {
                image_path = body["image_path"].get<std::string>();
            } else if (body.contains("image") && body["image"].is_string()) {
                image_path = body["image"].get<std::string>();
            }
        }

        if (image_path.empty()) {
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody(json{{"error", "image_path parameter is required"}}.dump());
            cb(resp);
            return;
        }

        if (image_path == "." || image_path == ".." || image_path.find("../") != std::string::npos
            || image_path.find("..\\") != std::string::npos) {
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody(json{{"error", "invalid image path"}}.dump());
            cb(resp);
            return;
        }

        if (!std::filesystem::exists(image_path)) {
            resp->setStatusCode(drogon::k404NotFound);
            resp->setBody(json{{"error", "image file not found"}}.dump());
            cb(resp);
            return;
        }

        std::vector<DetectedFaceInfo> faces;
        if (deps.detect_faces) { faces = deps.detect_faces(image_path); }

        json faces_arr = json::array();
        for (const auto& f : faces) { faces_arr.push_back(detected_face_to_json(f)); }

        resp->setBody(json{{"image", image_path}, {"faces", faces_arr}}.dump());
        cb(resp);
    };
    app.registerHandler("/api/faces", handle_faces, {drogon::Get, drogon::Post});

    // POST /api/upload (binary body + X-File-Name header)
    app.registerHandler(
        "/api/upload",
        [tasks, temp_dir](const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            auto raw_name = req->getHeader("X-File-Name");
            if (raw_name.empty()) {
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody(json{{"error", "missing X-File-Name header"}}.dump());
                cb(resp);
                return;
            }
            auto clean_name = drogon::utils::urlDecode(raw_name);
            // Reject any path separators / traversal outright (defense in depth:
            // do NOT normalize via filename(), which would silently rewrite them).
            if (clean_name.empty() || clean_name == "." || clean_name == ".."
                || clean_name.find('/') != std::string::npos
                || clean_name.find('\\') != std::string::npos) {
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody(json{{"error", "invalid file name"}}.dump());
                cb(resp);
                return;
            }
            const auto& body = req->getBody();
            if (body.size() > kMaxUploadBytes) {
                resp->setStatusCode(drogon::k413RequestEntityTooLarge);
                resp->setBody(json{{"error", "file too large (max 1GB)"}}.dump());
                cb(resp);
                return;
            }
            std::error_code ec;
            std::filesystem::create_directories(temp_dir, ec);
            auto stamp =
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            auto out_path = std::filesystem::path(temp_dir) / (stamp + "_" + clean_name);
            std::ofstream out(out_path, std::ios::binary);
            if (!out) {
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody(json{{"error", "failed to write upload"}}.dump());
                cb(resp);
                return;
            }
            out.write(body.data(), static_cast<std::streamsize>(body.size()));
            out.close();
            resp->setStatusCode(drogon::k201Created);
            resp->setBody(
                json{{"path", out_path.string()}, {"name", clean_name}, {"size", body.size()}}
                    .dump());
            cb(resp);
        },
        {drogon::Post});

    // POST /api/tasks/{task_id}/priority
    app.registerHandler(
        "/api/tasks/{task_id}/priority",
        [tasks](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto task_id = path_param(req, 0);
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            auto entry = tasks->get(task_id);
            if (!entry) {
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "task not found"}}.dump());
                cb(resp);
                return;
            }
            json body;
            try {
                body = json::parse(req->getBody());
            } catch (const std::exception&) {
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody(json{{"error", "invalid JSON body"}}.dump());
                cb(resp);
                return;
            }
            if (!body.contains("priority") || !body["priority"].is_number_integer()) {
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody(json{{"error", "priority (integer) is required"}}.dump());
                cb(resp);
                return;
            }
            int priority = body["priority"].get<int>();
            if (!tasks->set_priority(task_id, priority)) {
                resp->setStatusCode(drogon::k409Conflict);
                resp->setBody(json{{"error", "task is not queued"}}.dump());
                cb(resp);
                return;
            }
            resp->setBody(json{{"ok", true}, {"priority", priority}}.dump());
            cb(resp);
        },
        {drogon::Post});

    // GET /api/tasks/{task_id}
    app.registerHandler("/api/tasks/{task_id}",
                        [tasks](const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto task_id = path_param(req, 0);
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            auto entry = tasks->get(task_id);
                            if (!entry) {
                                resp->setStatusCode(drogon::k404NotFound);
                                resp->setBody(json{{"error", "task not found"}}.dump());
                                cb(resp);
                                return;
                            }
                            resp->setBody(task_entry_to_json(*entry).dump());
                            cb(resp);
                        },
                        {drogon::Get});

    // GET /api/tasks/{task_id}/progress
    app.registerHandler("/api/tasks/{task_id}/progress",
                        [tasks](const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto task_id = path_param(req, 0);
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            auto entry = tasks->get(task_id);
                            if (!entry) {
                                resp->setStatusCode(drogon::k404NotFound);
                                resp->setBody(json{{"error", "task not found"}}.dump());
                                cb(resp);
                                return;
                            }
                            json j = {{"id", entry->id},
                                      {"status", status_to_string(entry->status)},
                                      {"progress", progress_to_json(entry->progress)},
                                      {"error_message", entry->error_message}};
                            resp->setBody(j.dump());
                            cb(resp);
                        },
                        {drogon::Get});

    // POST /api/tasks/{task_id}/cancel
    app.registerHandler("/api/tasks/{task_id}/cancel",
                        [tasks](const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
                            auto task_id = path_param(req, 0);
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                            resp->setBody(json{{"ok", tasks->cancel(task_id)}}.dump());
                            cb(resp);
                        },
                        {drogon::Post});

    // GET /api/tasks/{task_id}/result
    app.registerHandler(
        "/api/tasks/{task_id}/result",
        [tasks](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto task_id = path_param(req, 0);
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
            auto entry = tasks->get(task_id);
            if (!entry) {
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "task not found"}}.dump());
                cb(resp);
                return;
            }
            json files = json::array();
            for (const auto& f : entry->result_files) {
                files.push_back({{"name", f}, {"url", "/media/" + task_id + "/result/" + f}});
            }
            resp->setBody(json{{"files", files}}.dump());
            cb(resp);
        },
        {drogon::Get});

    // GET /media/{task_id}/{kind}/{name}
    app.registerHandler(
        "/media/{task_id}/{kind}/{name}",
        [tasks](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            auto task_id = path_param(req, 0);
            auto kind = path_param(req, 1);
            auto name = path_param(req, 2);

            auto entry = tasks->get(task_id);
            if (!entry) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "task not found"}}.dump());
                cb(resp);
                return;
            }

            std::filesystem::path file_path;
            if (kind == "source") {
                std::size_t idx = 0;
                try {
                    idx = std::stoul(name);
                } catch (...) { idx = static_cast<std::size_t>(-1); }
                if (idx >= entry->config.io.source_paths.size()) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody(json{{"error", "invalid index"}}.dump());
                    cb(resp);
                    return;
                }
                file_path = entry->config.io.source_paths[idx];
            } else if (kind == "target") {
                std::size_t idx = 0;
                try {
                    idx = std::stoul(name);
                } catch (...) { idx = static_cast<std::size_t>(-1); }
                if (idx >= entry->config.io.target_paths.size()) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody(json{{"error", "invalid index"}}.dump());
                    cb(resp);
                    return;
                }
                file_path = entry->config.io.target_paths[idx];
            } else if (kind == "result") {
                if (name.find('/') != std::string::npos || name.find("\\") != std::string::npos
                    || name == ".." || name == ".") {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody(json{{"error", "invalid file name"}}.dump());
                    cb(resp);
                    return;
                }
                file_path = std::filesystem::path(entry->config.io.output.path) / name;
            } else {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "invalid media kind"}}.dump());
                cb(resp);
                return;
            }

            // Whitelist check: result files must live inside the task output dir
            if (kind == "result"
                && !is_path_within(file_path,
                                   std::filesystem::path(entry->config.io.output.path))) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "forbidden"}}.dump());
                cb(resp);
                return;
            }
            if (!std::filesystem::exists(file_path)) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody(json{{"error", "file not found"}}.dump());
                cb(resp);
                return;
            }
            serve_file(file_path, req, std::move(cb));
        },
        {drogon::Get});

    // Static hosting: Drogon serves unmatched GET routes from document root
    if (!options.web_root.empty() && std::filesystem::is_directory(options.web_root)) {
        app.setDocumentRoot(options.web_root);
    }

    app.run();
}

} // namespace app::web
