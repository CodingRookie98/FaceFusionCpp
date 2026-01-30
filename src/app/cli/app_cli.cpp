module;
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <indicators/progress_bar.hpp>
#include <CLI/CLI.hpp>

module app.cli;

import config.parser;
import services.pipeline.runner;
import foundation.infrastructure.logger;

namespace app::cli {

// Global atomic flag for signal handling
std::atomic<bool> g_interrupted{false};
// Global pointer to active runner for cancellation
services::pipeline::PipelineRunner* g_active_runner = nullptr;

void signal_handler(int signal) {
    using foundation::infrastructure::logger::Logger;
    if (signal == SIGINT) {
        Logger::get_instance()->warn("Interrupt received (SIGINT). Stopping pipeline...");
        g_interrupted = true;
        if (g_active_runner) { g_active_runner->Cancel(); }
    }
}

void App::run_pipeline(const std::string& config_path) {
    using namespace config;
    using namespace services::pipeline;
    using foundation::infrastructure::logger::Logger;

    auto logger = Logger::get_instance();

    // 1. Load Config
    auto config_result = LoadTaskConfig(config_path);
    if (config_result.is_err()) {
        logger->error("Config Error: " + config_result.error().message);
        return;
    }
    auto task_config = config_result.value();

    // 2. Setup AppConfig (Default for now, later load from app_config.yaml)
    AppConfig app_config;
    app_config.models.path = "assets/models"; // Default

    // 3. Create Runner
    auto runner = CreatePipelineRunner(app_config);
    g_active_runner = runner.get();

    // Setup Progress Bar
    indicators::ProgressBar bar{indicators::option::BarWidth{50},
                                indicators::option::Start{"["},
                                indicators::option::Fill{"="},
                                indicators::option::Lead{">"},
                                indicators::option::Remainder{" "},
                                indicators::option::End{"]"},
                                indicators::option::PostfixText{"Initializing..."},
                                indicators::option::ForegroundColor{indicators::Color::green},
                                indicators::option::ShowElapsedTime{true},
                                indicators::option::ShowRemainingTime{true},
                                indicators::option::FontStyles{std::vector<indicators::FontStyle>{
                                    indicators::FontStyle::bold}}};

    // 4. Run
    logger->info("Starting task: " + task_config.task_info.id);

    auto result = runner->Run(task_config, [&](const TaskProgress& p) {
        float progress = 0.0f;
        if (p.total_frames > 0) { progress = (float)p.current_frame / p.total_frames * 100.0f; }

        bar.set_progress(progress);

        std::string postfix = "Frame: " + std::to_string(p.current_frame) + "/"
                            + std::to_string(p.total_frames) + " (" + std::to_string((int)p.fps)
                            + " FPS)";
        bar.set_option(indicators::option::PostfixText{postfix});
    });

    // Cleanup
    g_active_runner = nullptr;

    if (g_interrupted) {
        logger->warn("Task cancelled by user.");
    } else if (result.is_err()) {
        logger->error("Pipeline failed: " + result.error().message);
    } else {
        bar.set_progress(100.0f);
        bar.set_option(indicators::option::PostfixText{"Completed"});
        logger->info("Task completed successfully.");
    }
}

int App::run(int argc, char** argv) {
    using foundation::infrastructure::logger::Logger;
    auto logger = Logger::get_instance();

    CLI::App app{"FaceFusionCpp"};
// Ensure UTF-8 support for arguments
#ifdef _WIN32
    argv = app.ensure_utf8(argv);
#endif

    std::string config_path;
    bool show_version = false;

    app.add_option("-c,--config", config_path, "Path to task configuration file");
    app.add_flag("-v,--version", show_version, "Show version information");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) { return app.exit(e); }

    if (show_version) {
        print_version();
        return 0;
    }

    if (!config_path.empty()) {
        // Register signal handler
        std::signal(SIGINT, signal_handler);
        run_pipeline(config_path);
        return 0;
    }

    // Show help if no action taken
    std::cout << app.help() << std::endl;
    return 0;
}

void App::print_version() {
    std::cout << "FaceFusionCpp v1.0.0" << std::endl;
}

} // namespace app::cli
