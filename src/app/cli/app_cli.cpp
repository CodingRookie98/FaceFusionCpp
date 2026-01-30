module;
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <indicators/progress_bar.hpp>

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
    if (signal == SIGINT) {
        std::cout << "\nInterrupt received (SIGINT). Stopping pipeline...\n";
        g_interrupted = true;
        if (g_active_runner) { g_active_runner->Cancel(); }
    }
}

int App::run(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    if (args.empty()) {
        print_help();
        return 0;
    }

    // Register signal handler
    std::signal(SIGINT, signal_handler);

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--help" || args[i] == "-h") {
            print_help();
            return 0;
        }
        if (args[i] == "--version" || args[i] == "-v") {
            print_version();
            return 0;
        }
        if (args[i] == "--config" || args[i] == "-c") {
            if (i + 1 < args.size()) {
                run_pipeline(args[i + 1]);
                return 0;
            } else {
                std::cerr << "Error: --config requires a path argument" << std::endl;
                return 1;
            }
        }
    }

    std::cout << "Unknown command. Use --help to see available options." << std::endl;
    return 1;
}

void App::run_pipeline(const std::string& config_path) {
    using namespace config;
    using namespace services::pipeline;

    // 1. Load Config
    auto config_result = LoadTaskConfig(config_path);
    if (config_result.is_err()) {
        std::cerr << "Config Error: " << config_result.error().message << std::endl;
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
    std::cout << "Starting task: " << task_config.task_info.id << std::endl;

    auto result = runner->Run(task_config, [&](const TaskProgress& p) {
        float progress = 0.0f;
        if (p.total_frames > 0) { progress = (float)p.current_frame / p.total_frames * 100.0f; }

        bar.set_progress(progress);

        std::string postfix = "Frame: " + std::to_string(p.current_frame) + "/"
                            + std::to_string(p.total_frames) + " (" + std::to_string((int)p.fps)
                            + " FPS)";
        bar.set_option(indicators::option::PostfixText{postfix});

        if (g_interrupted) {
            // If interrupted, we might want to break here or let the runner handle cancel
        }
    });

    // Cleanup
    g_active_runner = nullptr;

    if (g_interrupted) {
        std::cout << "\nTask cancelled by user." << std::endl;
    } else if (result.is_err()) {
        std::cerr << "\nPipeline failed: " << result.error().message << std::endl;
    } else {
        bar.set_progress(100.0f);
        bar.set_option(indicators::option::PostfixText{"Completed"});
        std::cout << "\nTask completed successfully." << std::endl;
    }
}

void App::print_help() {
    std::cout << "Usage: FaceFusionCpp [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "  -c, --config     Path to task configuration file" << std::endl;
}

void App::print_version() {
    std::cout << "FaceFusionCpp v1.0.0" << std::endl;
}

} // namespace app::cli
