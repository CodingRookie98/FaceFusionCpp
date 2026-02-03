/**
 * @file config_merger.ixx
 * @brief Configuration merging utilities
 * @see design.md Section 3.1 - Config Cascade Priority (Task > User > Default)
 * @see design.md V2.6 - 配置级联规范
 */
module;

#include <string>

export module config.merger;

export import config.app;
export import config.task;

export namespace config {

/**
 * @brief Merge AppConfig defaults into TaskConfig
 *
 * Priority: TaskConfig explicit values > AppConfig defaults > Hardcoded defaults
 * @see design.md Section 3.1 - 配置级联: Task Config > User Config > System Default
 *
 * @param task The task configuration (may have empty/default fields)
 * @param app The application configuration with default_task_settings
 * @return TaskConfig with all fields populated
 */
[[nodiscard]] TaskConfig MergeConfigs(const TaskConfig& task, const AppConfig& app);

/**
 * @brief Apply default model names to pipeline steps
 *
 * If a pipeline step has an empty model field, fill it from AppConfig.default_models
 */
void ApplyDefaultModels(TaskConfig& task, const DefaultModels& defaults);

} // namespace config
