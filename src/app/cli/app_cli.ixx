module;
#include <string>
#include <vector>
#include <iostream>
#include <optional>

export module app.cli;

import config.parser;
import services.pipeline.runner;
import foundation.infrastructure.logger;

export namespace app::cli {

class App {
public:
    static int run(int argc, char** argv);

private:
    static void print_version();
    static void print_startup_banner();
    static void log_config_summary(const config::AppConfig& app_config);
    static void log_hardware_info();

    static int run_pipeline(const std::string& config_path, const config::AppConfig& app_config);
    static int run_pipeline_internal(const config::TaskConfig& task_config,
                                     const config::AppConfig& app_config);

    static int run_system_check(bool json_output);
    static int run_validate(const std::string& config_path, const config::AppConfig& app_config);

    static int run_quick_mode(const std::vector<std::string>& source_paths,
                              const std::vector<std::string>& target_paths,
                              const std::string& output_path, const std::string& processors_str,
                              const config::AppConfig& app_config);

    static std::optional<config::AppConfig> load_app_config(const std::string& path,
                                                            const std::string& log_level_override);
};

} // namespace app::cli
