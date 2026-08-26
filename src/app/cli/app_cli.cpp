module;
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <CLI/CLI.hpp>
#include <format>
#include <filesystem>
#include <sstream>
#include <opencv2/opencv.hpp>

module app.cli;

import config.parser;
import config.merger;
import config.validator;
import services.pipeline.runner;
import services.pipeline.shutdown;
import foundation.infrastructure.logger;
import foundation.infrastructure.core_utils;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import domain.ai.model_repository;
import domain.face.model_registry;
import domain.face;
import domain.face.analyser;
import domain.face.detector;
import domain.face.recognizer;
import domain.common;
import domain.pipeline;
import app.cli.system_check;
import app.version;
import foundation.infrastructure.progress;
import processor.param_registry;
import app.web.server;
import app.web.task_manager;
import app.web.task_types;
import app.web.pipeline_executor;

namespace app::cli {

namespace {
config::LogLevel parse_log_level(const std::string& level) {
    auto r = config::parse_log_level(level);
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

// ─────────────────────────────────────────────────────────────────────────
// Helper: Convert snake_case to kebab-case for CLI flag names
// ─────────────────────────────────────────────────────────────────────────
namespace {
std::string to_kebab(const std::string& snake) {
    std::string result;
    for (char c : snake) { result += (c == '_') ? '-' : c; }
    return result;
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────
// Helper: Register processor params as CLI11 flags from registry
// ─────────────────────────────────────────────────────────────────────────
static ProcessorParamMap register_processor_cli_params(CLI::App& cli_app) {
    using namespace domain::processor;
    ProcessorParamMap result;
    auto& registry = ProcessorParamRegistry::instance();

    for (const auto& proc_name : registry.all_processor_names()) {
        auto* meta = registry.find(proc_name);
        if (!meta) continue;

        for (const auto& param : meta->params) {
            std::string cli_flag = "--" + to_kebab(proc_name) + "-" + to_kebab(param.name);
            std::string* storage = &result[proc_name][param.name];

            switch (param.type) {
            case ParamType::Bool:
                cli_app.add_flag(cli_flag, *storage, param.description)->excludes("--task-config");
                break;
            default: {
                auto* opt = cli_app.add_option(cli_flag, *storage, param.description);
                opt->excludes("--task-config");
                if (!param.allowed_values.empty()) {
                    opt->check(CLI::IsMember(param.allowed_values));
                }
                if (param.range) {
                    opt->check(CLI::Range(param.range->first, param.range->second));
                }
                break;
            }
            }
        }
    }
    return result;
}

int App::run(int argc, char** argv) {
    CLI::App app{"ffc (FaceFusionCpp) - Face processing pipeline"};

#ifdef _WIN32
    argv = app.ensure_utf8(argv);
#endif

    // ─────────────────────────────────────────────────────────────────────────
    // 全局选项 (Global Options)
    // ─────────────────────────────────────────────────────────────────────────
    std::string config_path;
    std::string app_config_path; // 留空以启用自动寻径
    std::string log_level;
    bool show_version = false;
    bool validate_only = false;
    bool system_check = false;
    bool json_output = false;

    app.add_option("-c,--task,--task-config", config_path, "Path to task configuration file");
    app.add_option(
        "--app-config", app_config_path,
        "Path to application config (default: searches executable/working dir config/app.yaml)");
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

    app.add_option("-s,--source", source_paths, "Source face image(s)")
        ->excludes("-c")
        ->excludes("--task")
        ->excludes("--task-config");
    app.add_option("-t,--target", target_paths, "Target image/video path(s)")
        ->excludes("-c")
        ->excludes("--task")
        ->excludes("--task-config");
    app.add_option("-o,--output", output_path, "Output directory or file path")
        ->excludes("-c")
        ->excludes("--task")
        ->excludes("--task-config");
    app.add_option("--processors", processors_str,
                   "Comma-separated processor list "
                   "(face_swapper,face_enhancer,expression_restorer,frame_enhancer)")
        ->excludes("-c")
        ->excludes("--task")
        ->excludes("--task-config");

    // ─────────────────────────────────────────────────────────────────────────
    // Web 模式选项 (Web UI Mode Options)
    // 注意: 必须在快捷模式选项之后注册（excludes 引用的选项需已存在）
    // ─────────────────────────────────────────────────────────────────────────
    bool web_mode = false;
    uint16_t web_port = 8000;
    std::string web_host = "0.0.0.0";
    std::string web_root = "assets/web";

    app.add_flag("--web", web_mode, "Run the embedded web UI server")
        ->excludes("-c")
        ->excludes("--task")
        ->excludes("--task-config")
        ->excludes("--source")
        ->excludes("--target")
        ->excludes("--output")
        ->excludes("--processors");
    app.add_option("--web-port", web_port, "Web server port")->check(CLI::Range(1, 65535));
    app.add_option("--web-host", web_host, "Web server bind host");
    app.add_option("--web-root", web_root, "Frontend static assets root");

    // Register processor-specific CLI flags from metadata registry
    auto processor_params = register_processor_cli_params(app);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) { return app.exit(e); }

    // ─────────────────────────────────────────────────────────────────────────
    // 处理流程
    // ─────────────────────────────────────────────────────────────────────────

    int exit_code = 0;
    if (show_version) {
        print_version();
        exit_code = 0;
    } else if (system_check) {
        exit_code = run_system_check(json_output);
    } else {
        // 加载 AppConfig
        auto app_config = load_app_config(app_config_path, log_level);
        if (!app_config) {
            std::cerr << "Failed to load app config: " << app_config_path << std::endl;
            return 1;
        }

        // 初始化 ModelRepository
        auto model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        model_repo->set_base_path(app_config->models.path);
        // 假设 models_info.json 位于 models.path 的父目录 (例如 assets/models_info.json)
        // 或者直接位于 models.path (如果配置指向 assets)
        // 这里采用保守策略：如果 models.path 是 assets/models，则找 assets/models_info.json
        std::filesystem::path models_path(app_config->models.path);
        std::filesystem::path info_path = models_path.parent_path() / "models_info.json";
        if (std::filesystem::exists(info_path)) {
            model_repo->set_model_info_file_path(info_path.string());
        } else {
            // Fallback: try inside models path
            info_path = models_path / "models_info.json";
            if (std::filesystem::exists(info_path)) {
                model_repo->set_model_info_file_path(info_path.string());
            } else {
                foundation::infrastructure::logger::Logger::get_instance()->warn(
                    "models_info.json not found in " + models_path.parent_path().string() + " or "
                    + models_path.string());
            }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 启动日志序列 (符合 design.md 5.10.1 INFO 级必选场景)
        // ─────────────────────────────────────────────────────────────────────────
        print_startup_banner();          // 1. 版本 Banner
        log_config_summary(*app_config); // 2. 配置摘要
        log_hardware_info();             // 3. 硬件信息

        if (validate_only) {
            config::TaskConfig task_config;
            bool has_config = false;
            if (!config_path.empty()) {
                exit_code = run_validate_from_file(config_path, *app_config);
            } else if (!source_paths.empty() && !target_paths.empty()) {
                task_config = build_quick_task_config(source_paths, target_paths, output_path,
                                                      processors_str, processor_params);
                has_config = true;
            } else {
                std::cerr << "Error: --validate requires --task-config or (-s, -t)" << '\n';
                exit_code = 1;
            }
            if (has_config && exit_code == 0) {
                exit_code = run_validate(task_config, *app_config);
            }
        } else if (web_mode) {
            std::string effective_host = app.count("--web-host") ? web_host : app_config->web.host;
            uint16_t effective_port = app.count("--web-port") ? web_port : app_config->web.port;
            std::string effective_web_root =
                app.count("--web-root") ? web_root : app_config->web.web_root;
            exit_code =
                run_web_mode(effective_host, effective_port, effective_web_root, *app_config);
        } else if (!config_path.empty()) {
            exit_code = run_pipeline(config_path, *app_config);
        } else if (!source_paths.empty() && !target_paths.empty()) {
            exit_code = run_quick_mode(source_paths, target_paths, output_path, processors_str,
                                       processor_params, *app_config);
        } else {
            std::cout << app.help() << '\n';
            exit_code = 0;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 清理资源 (重要：避免 CUDA 驱动提前关闭导致的析构崩溃)
        // ─────────────────────────────────────────────────────────────────────────
        domain::face::FaceModelRegistry::get_instance()->clear();
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->clear();
    }

    return exit_code;
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

int App::run_web_mode(const std::string& host, uint16_t port, const std::string& web_root,
                      const config::AppConfig& app_config) {
    using foundation::infrastructure::logger::Logger;
    Logger::get_instance()->info(
        std::format("Web UI starting: http://{}:{}/ (web root: {})", host, port, web_root));

    // Ensure builtin adapters are registered
    domain::pipeline::register_builtin_adapters();

    // Wire the production task executor (PipelineRunner) into the task manager
    auto executor = std::make_shared<app::web::PipelineTaskExecutor>(app_config);
    auto tasks = std::make_shared<app::web::TaskManager>(executor);

    // Production face detector hook backed by FaceAnalyser
    auto detect_faces =
        [app_config](const std::string& image_path) -> std::vector<app::web::DetectedFaceInfo> {
        using foundation::infrastructure::logger::Logger;
        try {
            cv::Mat frame = cv::imread(image_path);
            if (frame.empty()) {
                Logger::get_instance()->warn(
                    std::format("detect_faces: failed to read image: {}", image_path));
                return {};
            }

            auto model_repo = domain::ai::model_repository::ModelRepository::get_instance();
            domain::face::analyser::Options opts;
            opts.inference_session_options =
                foundation::ai::inference_session::Options::with_best_providers();
            opts.model_paths.face_detector_yolo =
                model_repo->ensure_model(app_config.default_models.face_detector);
            opts.model_paths.face_recognizer_arcface =
                model_repo->ensure_model(app_config.default_models.face_recognizer);
            opts.face_detector_options.type = domain::face::detector::DetectorType::Yolo;
            opts.face_recognizer_type =
                domain::face::recognizer::FaceRecognizerType::ArcFaceW600kR50;

            static std::mutex s_analyser_mutex;
            static std::shared_ptr<domain::face::analyser::FaceAnalyser> s_analyser;
            std::shared_ptr<domain::face::analyser::FaceAnalyser> analyser;
            {
                std::lock_guard lock(s_analyser_mutex);
                if (!s_analyser) {
                    s_analyser = std::make_shared<domain::face::analyser::FaceAnalyser>(opts);
                }
                analyser = s_analyser;
            }

            auto faces = analyser->get_many_faces(
                frame, domain::face::analyser::FaceAnalysisType::Detection
                           | domain::face::analyser::FaceAnalysisType::Landmark);

            std::vector<app::web::DetectedFaceInfo> results;
            results.reserve(faces.size());
            for (std::size_t i = 0; i < faces.size(); ++i) {
                const auto& f = faces[i];
                app::web::DetectedFaceInfo info;
                info.index = static_cast<int>(i);
                info.box = {f.box().x, f.box().y, f.box().width, f.box().height};
                info.score = f.detector_score();
                info.gender =
                    (f.gender() == domain::common::types::Gender::Female) ? "female" : "male";
                info.age_range = {static_cast<int>(f.age_range().min),
                                  static_cast<int>(f.age_range().max)};
                for (const auto& pt : f.kps()) { info.kps.push_back({pt.x, pt.y}); }
                results.push_back(std::move(info));
            }
            return results;
        } catch (const std::exception& e) {
            Logger::get_instance()->error(std::format("detect_faces exception: {}", e.what()));
            return {};
        } catch (...) {
            Logger::get_instance()->error("detect_faces unknown exception");
            return {};
        }
    };

    app::web::run_server({.host = host, .port = port, .web_root = web_root},
                         {.tasks = tasks,
                          .app_config = &app_config,
                          .detect_faces = detect_faces,
                          .executor = executor});
    return 0;
}

int App::run_validate_from_file(const std::string& config_path,
                                const config::AppConfig& app_config) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    Logger::get_instance()->info("Validating configuration: " + config_path);

    // 1. 加载配置
    auto config_result = load_task_config(config_path);
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
    auto config_result = load_task_config(config_path);
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
                        const ProcessorParamMap& processor_params,
                        const config::AppConfig& app_config) {
    // Build TaskConfig from CLI params
    auto task_config = build_quick_task_config(source_paths, target_paths, output_path,
                                               processors_str, processor_params);

    // Merge with app defaults
    task_config = config::MergeConfigs(task_config, app_config);

    // Run
    return run_pipeline_internal(task_config, app_config);
}

int App::run_pipeline_internal(const config::TaskConfig& task_config,
                               const config::AppConfig& app_config) {
    using namespace config;
    using namespace services::pipeline;
    using foundation::infrastructure::logger::Logger;

    auto logger = Logger::get_instance();

    // 1. Create Runner
    std::shared_ptr<PipelineRunner> runner = create_pipeline_runner(app_config);

    // 2. Install shutdown handler
    ShutdownHandler::install(
        [runner]() {
            runner->cancel();
            (void)runner->wait_for_completion(std::chrono::seconds{10});
            ShutdownHandler::mark_completed();
        },
        std::chrono::seconds{5}, // timeout
        []() {
            Logger::get_instance()->error("Force terminating due to timeout");
            std::exit(1);
        });

    // 3. Setup Progress Bar
    foundation::infrastructure::progress::ProgressBar bar{"Initializing..."};

    // 4. Run
    logger->info("Starting task: " + task_config.task_info.id);

    std::string last_progress_info;
    auto result = runner->run(task_config, [&](const TaskProgress& p) {
        float progress = 0.0f;
        if (p.total_frames > 0) { progress = (float)p.current_frame / p.total_frames * 100.0f; }

        bar.set_progress(progress);

        last_progress_info =
            std::format("Frame: {}/{} ({:.1f} FPS)", p.current_frame, p.total_frames, p.fps);
        bar.set_postfix_text(last_progress_info);
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
        bar.set_postfix_text(std::format("{} Completed", last_progress_info));
        bar.mark_as_completed();
        logger->info("Task completed successfully.");
        return 0;
    }
}

static std::filesystem::path get_executable_dir() {
#if defined(_WIN32)
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif defined(__linux__)
    std::error_code ec;
    auto p = std::filesystem::canonical("/proc/self/exe", ec);
    if (!ec) { return p.parent_path(); }
    return std::filesystem::current_path();
#elif defined(__APPLE__)
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::filesystem::canonical(path).parent_path();
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

std::optional<config::AppConfig> App::load_app_config(const std::string& path,
                                                      const std::string& log_level_override) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    AppConfig config;
    std::string resolved_path = path;

    if (resolved_path.empty()) {
        auto exe_dir = get_executable_dir();
        std::vector<std::filesystem::path> candidates = {exe_dir / "config" / "app.yaml",
                                                         exe_dir / "app.yaml",
                                                         "config/app.yaml",
                                                         "app.yaml",
                                                         exe_dir / "config" / "app_config.yaml",
                                                         exe_dir / "app_config.yaml",
                                                         "config/app_config.yaml",
                                                         "app_config.yaml"};
        for (const auto& c : candidates) {
            if (std::filesystem::exists(c)) {
                resolved_path = c.string();
                break;
            }
        }
    }

    // 尝试从文件加载
    if (!resolved_path.empty() && std::filesystem::exists(resolved_path)) {
        auto result = config::load_app_config(resolved_path);
        if (result.is_ok()) {
            config = std::move(result).value();
        } else {
            // 配置文件存在但解析/版本校验失败: 拒绝启动 (design.md §3.3.1 启动时校验)
            std::cerr << "Failed to load app config (" << resolved_path
                      << "): " << result.error().message << std::endl;
            return std::nullopt;
        }
    }

    // 应用日志级别覆盖
    if (!log_level_override.empty()) { config.logging.level = parse_log_level(log_level_override); }

    // 初始化 Logger
    Logger::initialize(convert_logging_config(config.logging));

    return config;
}

void App::print_version() {
#ifdef _WIN32
    std::cout << foundation::infrastructure::core_utils::encoding::utf8_to_sys_default_local(
        app::version::get_version_string())
              << std::endl;
#else
    std::cout << app::version::get_version_string() << std::endl;
#endif
}

void App::print_startup_banner() {
    using foundation::infrastructure::logger::Logger;
#ifdef _WIN32
    Logger::get_instance()->info(
        foundation::infrastructure::core_utils::encoding::utf8_to_sys_default_local(
            app::version::get_banner()));
#else
    Logger::get_instance()->info(app::version::get_banner());
#endif
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
    logger->info(std::format("  Log Level: {}", config::to_string(app_config.logging.level)));
    logger->info(std::format("  Models Path: {}", app_config.models.path));
    logger->info(std::format("  Engine Cache: {} (Path: {}, Max: {}, TTL: {}s)",
                             app_config.inference.engine_cache.enable ? "Enabled" : "Disabled",
                             app_config.inference.engine_cache.path,
                             app_config.inference.engine_cache.max_entries,
                             app_config.inference.engine_cache.idle_timeout_seconds));
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

int App::run_validate(const config::TaskConfig& task_config, const config::AppConfig& app_config) {
    using namespace config;
    using foundation::infrastructure::logger::Logger;

    Logger::get_instance()->info("Validating configuration...");

    // Merge with app defaults
    auto merged = MergeConfigs(task_config, app_config);

    // Run validator
    ConfigValidator validator;
    auto errors = validator.validate(merged);

    if (errors.empty()) {
        std::cout << "Configuration valid.\n";
        return 0;
    }

    std::cout << "Validation failed with " << errors.size() << " error(s):\n";
    for (const auto& err : errors) { std::cout << err.to_config_error().formatted() << "\n"; }

    return static_cast<int>(errors[0].code);
}

config::TaskConfig App::build_quick_task_config(const std::vector<std::string>& source_paths,
                                                const std::vector<std::string>& target_paths,
                                                const std::string& output_path,
                                                const std::string& processors_str,
                                                const ProcessorParamMap& processor_params) {
    using namespace config;

    TaskConfig task_config;
    std::string uuid = foundation::infrastructure::core_utils::random::generate_uuid();
    std::replace(uuid.begin(), uuid.end(), '-', '_');
    task_config.task_info.id = "quick_" + uuid;
    task_config.io.source_paths = source_paths;
    task_config.io.target_paths = target_paths;
    task_config.io.output.path = output_path.empty() ? "./output/" : output_path;

    // Parse processor names
    std::vector<std::string> processors;
    if (processors_str.empty()) {
        processors = {"face_swapper"};
    } else {
        std::stringstream ss(processors_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(0, item.find_first_not_of(" \\t"));
            item.erase(item.find_last_not_of(" \\t") + 1);
            if (!item.empty()) { processors.push_back(item); }
        }
    }

    // Build pipeline steps with CLI params
    for (const auto& proc : processors) {
        PipelineStep step;
        step.step = proc;
        step.enabled = true;

        auto it = processor_params.find(proc);
        if (it != processor_params.end()) {
            for (const auto& [param_name, param_value] : it->second) {
                if (!param_value.empty()) { step.cli_params[param_name] = param_value; }
            }
        }

        // Convert raw CLI params into typed step params (merges with the YAML path).
        // CLI11 pre-validates values, so failure here is defensive only.
        auto apply_r = config::ApplyCliParamsToStep(step);
        if (!apply_r) {
            foundation::infrastructure::logger::Logger::get_instance()->warn(
                "Failed to apply CLI params for processor " + proc + ": "
                + apply_r.error().message);
        }

        task_config.pipeline.push_back(std::move(step));
    }

    return task_config;
}

} // namespace app::cli
