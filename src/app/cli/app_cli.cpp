module;
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <memory>
#include <indicators/progress_bar.hpp>
#include <CLI/CLI.hpp>
#include <format>
#include <filesystem>
#include <sstream>

module app.cli;

import config.parser;
import config.merger;
import services.pipeline.runner;
import services.pipeline.shutdown;
import foundation.infrastructure.logger;
import foundation.infrastructure.core_utils;
import app.cli.system_check;
import app.version;

namespace app::cli {

namespace {
config::LogLevel parse_log_level(const std::string& level) {
    auto r = config::ParseLogLevel(level);
    return r.is_ok() ? r.value() : config::LogLevel::Info;
}

foundation::infrastructure::logger::LoggingConfig convert_logging_config(
    const config::LoggingConfig& config) {
    using namespace foundation::infrastructure::logger;
    LoggingConfig result;

    // Convert LogLevel
    switch (config.level) {
    case config::LogLevel::Trace: result.level = LogLevel::Trace; break;
    case config::LogLevel::Debug: result.level = LogLevel::Debug; break;
    case config::LogLevel::Info: result.level = LogLevel::Info; break;
    case config::LogLevel::Warn: result.level = LogLevel::Warn; break;
    case config::LogLevel::Error: result.level = LogLevel::Error; break;
    default: result.level = LogLevel::Info; break;
    }

    result.directory = config.directory;

    // Convert RotationPolicy
    switch (config.rotation) {
    case config::LogRotation::Daily: result.rotation = RotationPolicy::Daily; break;
    case config::LogRotation::Hourly: result.rotation = RotationPolicy::Hourly; break;
    case config::LogRotation::Size: result.rotation = RotationPolicy::Size; break;
    default: result.rotation = RotationPolicy::Daily; break;
    }

    result.max_files = static_cast<uint32_t>(config.max_files);

    try {
        result.max_total_size_bytes = parse_size_string(config.max_total_size);
    } catch (...) { result.max_total_size_bytes = 1ULL << 30; }

    return result;
}
} // namespace

int App::run(int argc, char** argv) {
    CLI::App app{"FaceFusionCpp - Face processing pipeline"};

#ifdef _WIN32
    argv = app.ensure_utf8(argv);
#endif

    // ─────────────────────────────────────────────────────────────────────────
    // 全局选项 (Global Options)
    // ─────────────────────────────────────────────────────────────────────────
    std::string config_path;
    std::string app_config_path = "config/app_config.yaml"; // 默认路径
    std::string log_level;
    bool show_version = false;
    bool validate_only = false;
    bool system_check = false;
    bool json_output = false;

    app.add_option("-c,--config", config_path, "Path to task configuration file");
    app.add_option("--app-config", app_config_path, "Path to application config");
    app.add_option("--log-level", log_level, "Override log level (trace/debug/info/warn/error)")
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}));
    app.add_flag("-v,--version", show_version, "Show version information");
    app.add_flag("--validate", validate_only, "Validate config without execution");
    app.add_flag("--system-check", system_check, "Run system environment check");
    app.add_flag("--json", json_output, "Output in JSON format (with --system-check)");

    // ─────────────────────────────────────────────────────────────────────────
    // 快捷模式选项 (Quick Mode Options)
    // ─────────────────────────────────────────────────────────────────────────
    std::vector<std::string> source_paths;
    std::vector<std::string> target_paths;
    std::string output_path;
    std::string processors_str;

    app.add_option("-s,--source", source_paths, "Source face image(s)")->excludes("--config");
    app.add_option("-t,--target", target_paths, "Target image/video path(s)")->excludes("--config");
    app.add_option("-o,--output", output_path, "Output directory or file path")
        ->excludes("--config");
    app.add_option("--processors", processors_str,
                   "Comma-separated processor list "
                   "(face_swapper,face_enhancer,expression_restorer,frame_enhancer)")
        ->excludes("--config");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) { return app.exit(e); }

    // ─────────────────────────────────────────────────────────────────────────
    // 处理流程
    // ─────────────────────────────────────────────────────────────────────────

    if (show_version) {
        print_version();
        return 0;
    }

    if (system_check) { return run_system_check(json_output); }

    // 加载 AppConfig
    auto app_config = load_app_config(app_config_path, log_level);
    if (!app_config) {
        std::cerr << "Failed to load app config: " << app_config_path << std::endl;
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 启动日志序列 (符合 design.md 5.10.1 INFO 级必选场景)
    // ─────────────────────────────────────────────────────────────────────────
    print_startup_banner();          // 1. 版本 Banner
    log_config_summary(*app_config); // 2. 配置摘要
    log_hardware_info();             // 3. 硬件信息

    if (validate_only) {
        if (config_path.empty()) {
            std::cerr << "Error: --validate requires --config" << std::endl;
            return 1;
        }
        return run_validate(config_path, *app_config);
    }

    if (!config_path.empty()) { return run_pipeline(config_path, *app_config); }

    if (!source_paths.empty() && !target_paths.empty()) {
        return run_quick_mode(source_paths, target_paths, output_path, processors_str, *app_config);
    }

    std::cout << app.help() << std::endl;
    return 0;
}

int App::run_system_check(bool json_output) {
    auto report = run_all_checks();
    if (json_output) {
        std::cout << format_json(report) << std::endl;
    } else {
        std::cout << format_text(report) << std::endl;
    }
    return report.fail_count > 0 ? 1 : 0;
}

int App::run_validate(const std::string& config_path, const config::AppConfig& app_config) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    Logger::get_instance()->info("Validating configuration: " + config_path);

    // 1. 加载配置
    auto config_result = LoadTaskConfig(config_path);
    if (config_result.is_err()) {
        auto err = config_result.error();
        Logger::get_instance()->error(err.formatted());
        std::cerr << err.formatted() << std::endl;
        return static_cast<int>(err.code);
    }

    // 2. 合并配置
    auto task_config = MergeConfigs(config_result.value(), app_config);

    // 3. 运行校验器
    ConfigValidator validator;
    auto errors = validator.validate(task_config);

    if (errors.empty()) {
        std::cout << "Configuration valid: " << config_path << std::endl;
        return 0;
    }

    // 3. 输出所有错误
    std::cout << "Validation failed with " << errors.size() << " error(s):\n";
    for (const auto& err : errors) { std::cout << err.to_config_error().formatted() << "\n"; }

    return static_cast<int>(errors[0].code);
}

int App::run_pipeline(const std::string& config_path, const config::AppConfig& app_config) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    // 1. Load Task Config
    auto config_result = LoadTaskConfig(config_path);
    if (config_result.is_err()) {
        Logger::get_instance()->error("Config Error: " + config_result.error().message);
        return static_cast<int>(config_result.error().code);
    }
    auto task_config = MergeConfigs(config_result.value(), app_config);

    return run_pipeline_internal(task_config, app_config);
}

int App::run_quick_mode(const std::vector<std::string>& source_paths,
                        const std::vector<std::string>& target_paths,
                        const std::string& output_path, const std::string& processors_str,
                        const config::AppConfig& app_config) {
    using namespace config;

    // 1. 构建 TaskConfig
    TaskConfig task_config;
    std::string uuid = foundation::infrastructure::core_utils::random::generate_uuid();
    std::replace(uuid.begin(), uuid.end(), '-', '_');
    task_config.task_info.id = "quick_" + uuid;
    task_config.io.source_paths = source_paths;
    task_config.io.target_paths = target_paths;

    if (!output_path.empty()) {
        task_config.io.output.path = output_path;
    } else {
        // 默认输出目录
        task_config.io.output.path = "./output/";
    }

    // 2. 解析 processors
    std::vector<std::string> processors;
    if (processors_str.empty()) {
        processors = {"face_swapper"}; // 默认处理器
    } else {
        // Split by comma
        std::stringstream ss(processors_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            if (!item.empty()) { processors.push_back(item); }
        }
    }

    // 3. 添加 pipeline steps
    for (const auto& proc : processors) {
        PipelineStep step;
        step.step = proc;
        step.enabled = true;
        // 使用默认参数 (在 run_pipeline_internal 中会由 runner 处理或在 TaskConfig 中保留默认)
        task_config.pipeline.push_back(step);
    }

    // 4. 合并配置 (应用全局默认设置)
    task_config = MergeConfigs(task_config, app_config);

    // 5. 运行
    return run_pipeline_internal(task_config, app_config);
}

int App::run_pipeline_internal(const config::TaskConfig& task_config,
                               const config::AppConfig& app_config) {
    using namespace config;
    using namespace services::pipeline;
    using foundation::infrastructure::logger::Logger;

    auto logger = Logger::get_instance();

    // 1. Create Runner
    std::shared_ptr<PipelineRunner> runner = CreatePipelineRunner(app_config);

    // 2. Install shutdown handler
    ShutdownHandler::install(
        [runner]() {
            runner->Cancel();
            (void)runner->WaitForCompletion(std::chrono::seconds{10});
            ShutdownHandler::mark_completed();
        },
        std::chrono::seconds{5}, // timeout
        []() {
            Logger::get_instance()->error("Force terminating due to timeout");
            std::exit(1);
        });

    // 3. Setup Progress Bar
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

        std::string postfix =
            std::format("Frame: {}/{} ({:.1f} FPS)", p.current_frame, p.total_frames, p.fps);
        bar.set_option(indicators::option::PostfixText{postfix});
    });

    // Uninstall handler
    ShutdownHandler::uninstall();

    if (ShutdownHandler::is_shutdown_requested()) {
        logger->warn("Task cancelled by user.");
        return 1;
    } else if (result.is_err()) {
        logger->error("Pipeline failed: " + result.error().message);
        return static_cast<int>(result.error().code);
    } else {
        bar.set_progress(100.0f);
        bar.set_option(indicators::option::PostfixText{"Completed"});
        logger->info("Task completed successfully.");
        return 0;
    }
}

std::optional<config::AppConfig> App::load_app_config(const std::string& path,
                                                      const std::string& log_level_override) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    AppConfig config;

    // 尝试从文件加载
    if (std::filesystem::exists(path)) {
        auto result = LoadAppConfig(path);
        if (result.is_ok()) {
            config = std::move(result).value();
        } else {
            Logger::get_instance()->warn("Failed to load app config, using defaults: "
                                         + result.error().message);
        }
    }

    // 应用日志级别覆盖
    if (!log_level_override.empty()) { config.logging.level = parse_log_level(log_level_override); }

    // 初始化 Logger
    Logger::initialize(convert_logging_config(config.logging));

    return config;
}

void App::print_version() {
    std::cout << app::version::get_version_string() << std::endl;
}

void App::print_startup_banner() {
    using foundation::infrastructure::logger::Logger;
    Logger::get_instance()->info(app::version::get_banner());
}

void App::log_config_summary(const config::AppConfig& app_config) {
    using foundation::infrastructure::logger::Logger;
    auto logger = Logger::get_instance();

    logger->info("=== Configuration Summary ===");
    logger->info(std::format("  Device ID: {}", app_config.inference.device_id));
    logger->info(std::format("  Memory Strategy: {}",
                             app_config.resource.memory_strategy == config::MemoryStrategy::Strict ?
                                 "strict" :
                                 "tolerant"));
    logger->info(std::format("  Log Level: {}", config::ToString(app_config.logging.level)));
    logger->info(std::format("  Models Path: {}", app_config.models.path));
    logger->info("=============================");
}

void App::log_hardware_info() {
    using foundation::infrastructure::logger::Logger;
    auto logger = Logger::get_instance();

    // 复用 system_check 模块的检测逻辑
    auto report = run_all_checks();

    logger->info("=== Hardware Environment ===");
    for (const auto& check : report.checks) {
        if (check.status == CheckStatus::Ok) {
            logger->info(std::format("  {}: {}", check.name, check.value));
        } else if (check.status == CheckStatus::Warn) {
            logger->warn(std::format("  {}: {} ({})", check.name, check.value, check.message));
        } else {
            logger->error(std::format("  {}: {} ({})", check.name, check.value, check.message));
        }
    }
    logger->info("============================");
}

} // namespace app::cli
