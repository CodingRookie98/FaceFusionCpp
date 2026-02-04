/**
 * @file config_validator.ixx
 * @brief Enhanced configuration validation with YAML path localization
 * @see design.md Section 3.3 - Config Validation Mechanism
 */
module;

#include <string>
#include <vector>
#include <optional>

export module config.validator;

export import config.types;
export import config.task;
export import config.app;

export namespace config {

/**
 * @brief Validation error with full context
 */
struct ValidationError {
    ErrorCode code;
    std::string yaml_path;    ///< e.g., "pipeline[1].params.blend_factor"
    std::string actual_value; ///< The problematic value
    std::string expected;     ///< Expected constraint description
    int line = 0;
    int column = 0;

    /**
     * @brief Convert to ConfigError for Result types
     */
    [[nodiscard]] ConfigError to_config_error() const {
        const std::string msg = actual_value.empty() ? expected : actual_value + ", expected " + expected;
        return {code, msg, yaml_path, line, column};
    }
};

/**
 * @brief Configuration validator with comprehensive checks
 */
class ConfigValidator {
public:
    /**
     * @brief Validate TaskConfig and collect all errors
     * @param config The configuration to validate
     * @return Empty vector if valid, otherwise list of validation errors
     */
    [[nodiscard]] std::vector<ValidationError> validate(const TaskConfig& config);

    /**
     * @brief Validate AppConfig
     */
    [[nodiscard]] std::vector<ValidationError> validate(const AppConfig& config);

    /**
     * @brief Validate a single config and return Result
     * @return Ok if valid, Err with first error if invalid
     */
    [[nodiscard]] Result<void, ConfigError> validate_or_error(const TaskConfig& config);

    /**
     * @brief Validate AppConfig and return Result
     */
    [[nodiscard]] Result<void, ConfigError> validate_or_error(const AppConfig& config);

private:
    // ─────────────────────────────────────────────────────────────────────────
    // Validation Helpers
    // ─────────────────────────────────────────────────────────────────────────

    void validate_task_info(const TaskInfo& info, std::vector<ValidationError>& errors);

    void validate_io(const IOConfig& io, std::vector<ValidationError>& errors);

    void validate_face_analysis(const FaceAnalysisConfig& fa, std::vector<ValidationError>& errors);

    void validate_output(const OutputConfig& output, std::vector<ValidationError>& errors);

    void validate_pipeline(const std::vector<PipelineStep>& steps,
                           std::vector<ValidationError>& errors);

    void validate_step_params(const StepParams& params, const std::string& step_type,
                              const std::string& path_prefix, std::vector<ValidationError>& errors);

    // ─────────────────────────────────────────────────────────────────────────
    // Range Validators
    // ─────────────────────────────────────────────────────────────────────────

    template <typename TValue>
    void validate_range(TValue value, TValue min, TValue max, const std::string& yaml_path,
                        std::vector<ValidationError>& errors);

    void validate_path_exists(const std::string& path, const std::string& yaml_path,
                              std::vector<ValidationError>& errors);

    void validate_not_empty(const std::string& value, const std::string& yaml_path,
                            std::vector<ValidationError>& errors);
};

} // namespace config
