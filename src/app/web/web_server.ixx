/**
 * @file web_server.ixx
 * @brief Web UI server module (Drogon-based HTTP + static hosting)
 * @see docs/dev/zh/architecture/web_ui_design.md
 */
module;

#include <string>
#include <cstdint>

export module app.web.server;

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
 * @brief Generate the /api/health response JSON body
 * @return e.g. {"status":"ok","version":"0.34.1"}
 */
std::string health_json();

/**
 * @brief Run the embedded Drogon server (blocking until process exit)
 *
 * Routes:
 *  - GET /api/health -> health_json()
 *  - GET /           -> static files from web_root (if exists)
 */
void run_server(const WebServerOptions& options);

} // namespace app::web
