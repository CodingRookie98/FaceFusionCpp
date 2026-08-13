module;
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <map>

export module app.cli;

import config.parser;
import services.pipeline.runner;
import foundation.infrastructure.logger;

export namespace app::cli {

/// Map of {processor_name -> {param_name -> value}}
using ProcessorParamMap = std::map<std::string, std::map<std::string, std::string>>;

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

    /// Validate a TaskConfig (unified entry for both YAML and quick mode)
    static int run_validate(const config::TaskConfig& task_config,
                            const config::AppConfig& app_config);
    /// Validate from YAML file path
    static int run_validate_from_file(const std::string& config_path,
                                      const config::AppConfig& app_config);

    static int run_quick_mode(const std::vector<std::string>& source_paths,
                              const std::vector<std::string>& target_paths,
                              const std::string& output_path, const std::string& processors_str,
                              const ProcessorParamMap& processor_params,
                              const config::AppConfig& app_config);

    /// Build a TaskConfig from quick mode CLI parameters
    static config::TaskConfig build_quick_task_config(const std::vector<std::string>& source_paths,
                                                      const std::vector<std::string>& target_paths,
                                                      const std::string& output_path,
                                                      const std::string& processors_str,
                                                      const ProcessorParamMap& processor_params);

    static std::optional<config::AppConfig> load_app_config(const std::string& path,
                                                            const std::string& log_level_override);
};

} // namespace app::cli
