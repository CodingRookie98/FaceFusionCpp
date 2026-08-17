/**
 * @file web_server.ixx
 * @brief Web UI server module (Drogon-based HTTP + static hosting)
 * @see docs/dev/zh/architecture/web_ui_design.md
 */
module;

#include <string>
#include <cstdint>
#include <memory>

export module app.web.server;

import app.web.task_manager;
import config.app;

export namespace app::web {

/**
 * @brief Options for the embedded web server
 */
struct WebServerOptions {
    std::string host = "0.0.0.0"; ///< Bind address
    uint16_t port = 8000;         ///< Listen port
    std::string web_root = "assets/web"; ///< Frontend static assets root
};

/**
 * @brief Dependencies required by the web server
 */
struct WebServerDeps {
    std::shared_ptr<TaskManager> tasks;      ///< Task registry (required)
    const config::AppConfig* app_config = nullptr; ///< For merging task defaults (may be null in tests)
};

/**
 * @brief Generate the /api/health response JSON body
 * @return e.g. {"status":"ok","version":"0.34.1"}
 */
std::string health_json();

/**
 * @brief Run the embedded Drogon server (blocking until process exit)
 *
 * Routes:
 *  - GET  /api/health               -> health_json()
 *  - POST /api/tasks                -> submit task
 *  - GET  /api/tasks                -> task list
 *  - GET  /api/tasks/{id}           -> task detail (incl. media/result URLs)
 *  - POST /api/tasks/{id}/cancel    -> cancel task
 *  - GET  /api/tasks/{id}/result    -> result files
 *  - WS   /ws/tasks/{id}/progress   -> progress push
 *  - GET  /                         -> static files from web_root (if exists)
 */
void run_server(const WebServerOptions& options, const WebServerDeps& deps);

} // namespace app::web
