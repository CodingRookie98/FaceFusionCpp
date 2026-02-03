/**
 * @file config_validator.cpp
 * @brief Configuration validator implementation
 */
module;

#include <string>
#include <vector>
#include <filesystem>
#include <format>
#include <regex>
#include <algorithm>
#include <variant>

module config.validator;

namespace config {

std::vector<ValidationError> ConfigValidator::validate(const TaskConfig& config) {
    std::vector<ValidationError> errors;

    if (!config.config_version.empty() && config.config_version != SUPPORTED_CONFIG_VERSION) {
        errors.push_back({ErrorCode::E204_ConfigVersionMismatch, "config_version",
                          config.config_version,
                          std::format("supported version {}", SUPPORTED_CONFIG_VERSION)});
    }

    validate_task_info(config.task_info, errors);
    validate_io(config.io, errors);
    validate_face_analysis(config.face_analysis, errors);
    validate_pipeline(config.pipeline, errors);

    return errors;
}

std::vector<ValidationError> ConfigValidator::validate(const AppConfig& config) {
    std::vector<ValidationError> errors;

    if (config.config_version != SUPPORTED_CONFIG_VERSION) {
        errors.push_back({ErrorCode::E204_ConfigVersionMismatch, "config_version",
                          config.config_version,
                          std::format("supported version {}", SUPPORTED_CONFIG_VERSION)});
    }

    validate_path_exists(config.models.path, "models.path", errors);
    validate_not_empty(config.logging.directory, "logging.directory", errors);

    return errors;
}

Result<void, ConfigError> ConfigValidator::validate_or_error(const TaskConfig& config) {
    auto errors = validate(config);
    if (errors.empty()) { return Result<void, ConfigError>::Ok(); }
    return Result<void, ConfigError>::Err(errors[0].to_config_error());
}

Result<void, ConfigError> ConfigValidator::validate_or_error(const AppConfig& config) {
    auto errors = validate(config);
    if (errors.empty()) { return Result<void, ConfigError>::Ok(); }
    return Result<void, ConfigError>::Err(errors[0].to_config_error());
}

void ConfigValidator::validate_task_info(const TaskInfo& info,
                                         std::vector<ValidationError>& errors) {
    // Task ID format: [a-zA-Z0-9_]+
    if (!info.id.empty()) {
        std::regex id_regex("^[a-zA-Z0-9_]+$");
        if (!std::regex_match(info.id, id_regex)) {
            errors.push_back({ErrorCode::E202_ParameterOutOfRange, "task_info.id",
                              std::format("\"{}\"", info.id), "format [a-zA-Z0-9_]+"});
        }
    } else {
        errors.push_back(
            {ErrorCode::E205_RequiredFieldMissing, "task_info.id", "", "non-empty task id"});
    }
}

void ConfigValidator::validate_io(const IOConfig& io, std::vector<ValidationError>& errors) {
    // Source paths must not be empty
    if (io.source_paths.empty()) {
        errors.push_back({ErrorCode::E205_RequiredFieldMissing, "io.source_paths", "",
                          "at least one source path"});
    }

    // Validate source paths exist
    for (size_t i = 0; i < io.source_paths.size(); ++i) {
        validate_path_exists(io.source_paths[i], std::format("io.source_paths[{}]", i), errors);
    }

    // Target paths must not be empty
    if (io.target_paths.empty()) {
        errors.push_back({ErrorCode::E205_RequiredFieldMissing, "io.target_paths", "",
                          "at least one target path"});
    }

    // Validate target paths exist
    for (size_t i = 0; i < io.target_paths.size(); ++i) {
        validate_path_exists(io.target_paths[i], std::format("io.target_paths[{}]", i), errors);
    }

    validate_output(io.output, errors);
}

void ConfigValidator::validate_face_analysis(const FaceAnalysisConfig& fa,
                                             std::vector<ValidationError>& errors) {
    // Face detector score threshold: [0.0, 1.0]
    validate_range(fa.face_detector.score_threshold, 0.0, 1.0,
                   "face_analysis.face_detector.score_threshold", errors);

    // Face recognizer similarity threshold: [0.0, 1.0]
    validate_range(fa.face_recognizer.similarity_threshold, 0.0, 1.0,
                   "face_analysis.face_recognizer.similarity_threshold", errors);
}

void ConfigValidator::validate_output(const OutputConfig& output,
                                      std::vector<ValidationError>& errors) {
    // Output path must not be empty
    validate_not_empty(output.path, "io.output.path", errors);

    // Video quality range
    validate_range(output.video_quality, 0, 100, "io.output.video_quality", errors);

    // Image format validation
    static const std::vector<std::string> valid_formats = {"png", "jpg", "bmp", "jpeg"};
    std::string fmt = output.image_format;
    std::transform(fmt.begin(), fmt.end(), fmt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (std::find(valid_formats.begin(), valid_formats.end(), fmt) == valid_formats.end()) {
        errors.push_back({ErrorCode::E202_ParameterOutOfRange, "io.output.image_format",
                          std::format("\"{}\"", output.image_format),
                          "one of [png, jpg, jpeg, bmp]"});
    }
}

void ConfigValidator::validate_pipeline(const std::vector<PipelineStep>& steps,
                                        std::vector<ValidationError>& errors) {
    if (steps.empty()) {
        errors.push_back(
            {ErrorCode::E205_RequiredFieldMissing, "pipeline", "", "at least one pipeline step"});
        return;
    }

    static const std::vector<std::string> valid_steps = {"face_swapper", "face_enhancer",
                                                         "expression_restorer", "frame_enhancer"};

    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        std::string path_prefix = std::format("pipeline[{}]", i);

        // Validate step type
        std::string step_type = step.step;
        std::transform(step_type.begin(), step_type.end(), step_type.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (std::find(valid_steps.begin(), valid_steps.end(), step_type) == valid_steps.end()) {
            errors.push_back(
                {ErrorCode::E202_ParameterOutOfRange, path_prefix + ".step",
                 std::format("\"{}\"", step.step),
                 "one of [face_swapper, face_enhancer, expression_restorer, frame_enhancer]"});
        }

        // Validate params
        validate_step_params(step.params, step_type, path_prefix, errors);
    }
}

void ConfigValidator::validate_step_params(const StepParams& params, const std::string& step_type,
                                           const std::string& path_prefix,
                                           std::vector<ValidationError>& errors) {
    if (step_type == "face_swapper") {
        if (const auto* p = std::get_if<FaceSwapperParams>(&params)) {
            if (p->face_selector_mode == FaceSelectorMode::Reference) {
                if (!p->reference_face_path.has_value() || p->reference_face_path->empty()) {
                    errors.push_back({ErrorCode::E205_RequiredFieldMissing,
                                      path_prefix + ".params.reference_face_path", "",
                                      "required when face_selector_mode is 'reference'"});
                } else {
                    validate_path_exists(*p->reference_face_path,
                                         path_prefix + ".params.reference_face_path", errors);
                }
            }
        }
    } else if (step_type == "face_enhancer") {
        if (const auto* p = std::get_if<FaceEnhancerParams>(&params)) {
            validate_range(p->blend_factor, 0.0, 1.0, path_prefix + ".params.blend_factor", errors);
            if (p->face_selector_mode == FaceSelectorMode::Reference) {
                if (!p->reference_face_path.has_value() || p->reference_face_path->empty()) {
                    errors.push_back({ErrorCode::E205_RequiredFieldMissing,
                                      path_prefix + ".params.reference_face_path", "",
                                      "required when face_selector_mode is 'reference'"});
                } else {
                    validate_path_exists(*p->reference_face_path,
                                         path_prefix + ".params.reference_face_path", errors);
                }
            }
        }
    } else if (step_type == "expression_restorer") {
        if (const auto* p = std::get_if<ExpressionRestorerParams>(&params)) {
            validate_range(p->restore_factor, 0.0, 1.0, path_prefix + ".params.restore_factor",
                           errors);
            if (p->face_selector_mode == FaceSelectorMode::Reference) {
                if (!p->reference_face_path.has_value() || p->reference_face_path->empty()) {
                    errors.push_back({ErrorCode::E205_RequiredFieldMissing,
                                      path_prefix + ".params.reference_face_path", "",
                                      "required when face_selector_mode is 'reference'"});
                } else {
                    validate_path_exists(*p->reference_face_path,
                                         path_prefix + ".params.reference_face_path", errors);
                }
            }
        }
    } else if (step_type == "frame_enhancer") {
        if (const auto* p = std::get_if<FrameEnhancerParams>(&params)) {
            validate_range(p->enhance_factor, 0.0, 1.0, path_prefix + ".params.enhance_factor",
                           errors);
        }
    }
}

template <typename T>
void ConfigValidator::validate_range(T value, T min, T max, const std::string& yaml_path,
                                     std::vector<ValidationError>& errors) {
    if (value < min || value > max) {
        errors.push_back({ErrorCode::E202_ParameterOutOfRange, yaml_path, std::format("{}", value),
                          std::format("range [{}, {}]", min, max)});
    }
}

void ConfigValidator::validate_path_exists(const std::string& path, const std::string& yaml_path,
                                           std::vector<ValidationError>& errors) {
    if (path.empty()) return; // Handled by validate_not_empty if needed

    if (!std::filesystem::exists(path)) {
        errors.push_back({ErrorCode::E206_InvalidPath, yaml_path, std::format("\"{}\"", path),
                          "path must exist"});
    }
}

void ConfigValidator::validate_not_empty(const std::string& value, const std::string& yaml_path,
                                         std::vector<ValidationError>& errors) {
    if (value.empty()) {
        errors.push_back({ErrorCode::E205_RequiredFieldMissing, yaml_path, "", "non-empty string"});
    }
}

} // namespace config
