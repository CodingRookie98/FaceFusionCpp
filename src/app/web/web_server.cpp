module;
#include <string>
#include <filesystem>

#include <drogon/drogon.h>

module app.web.server;

import app.version;

namespace app::web {

std::string health_json() {
    return "{\"status\":\"ok\",\"version\":\"" + std::string(app::version::version()) + "\"}";
}

void run_server(const WebServerOptions& options) {
    auto& app = drogon::app();

    app.addListener(options.host, options.port);

    // REST: /api/health
    app.registerHandler("/api/health", [](const drogon::HttpRequestPtr&,
                                          std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(health_json());
        cb(resp);
    });

    // Static hosting: Drogon serves unmatched GET routes from document root
    // (including index.html at "/") once setDocumentRoot is configured.
    if (!options.web_root.empty() && std::filesystem::is_directory(options.web_root)) {
        app.setDocumentRoot(options.web_root);
    }

    app.run();
}

} // namespace app::web
