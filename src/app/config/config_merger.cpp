module;

#include <string>
#include <optional>
#include <variant>

module config.merger;

namespace config {

namespace {

// Helper: Apply default if target matches sentinel and default has value
template <typename T>
void ApplyIfMatch(T& target, const std::optional<T>& default_val, const T& sentinel) {
    if (default_val.has_value() && target == sentinel) { target = *default_val; }
}

// Specializations for common types
void ApplyIfEmpty(std::string& target, const std::optional<std::string>& default_val) {
    if (default_val.has_value() && target.empty()) { target = *default_val; }
}

void ApplyIfZero(int& target, const std::optional<int>& default_val) {
    if (default_val.has_value() && target == 0) { target = *default_val; }
}

void ApplyIfZero(double& target, const std::optional<double>& default_val) {
    if (default_val.has_value() && target == 0.0) { target = *default_val; }
}

} // namespace

TaskConfig MergeConfigs(const TaskConfig& task, const AppConfig& app) {
    TaskConfig result = task;
    const auto& defaults = app.default_task_settings;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. Merge IO Output settings
    // ─────────────────────────────────────────────────────────────────────────
    ApplyIfEmpty(result.io.output.video_encoder, defaults.io.output.video_encoder);
    ApplyIfZero(result.io.output.video_quality, defaults.io.output.video_quality);
    ApplyIfEmpty(result.io.output.prefix, defaults.io.output.prefix);
    ApplyIfEmpty(result.io.output.suffix, defaults.io.output.suffix);
    ApplyIfEmpty(result.io.output.image_format, defaults.io.output.image_format);

    if (defaults.io.output.conflict_policy.has_value()) {
        ApplyIfMatch(result.io.output.conflict_policy, defaults.io.output.conflict_policy,
                     ConflictPolicy::Error);
    }

    if (defaults.io.output.audio_policy.has_value()) {
        ApplyIfMatch(result.io.output.audio_policy, defaults.io.output.audio_policy,
                     AudioPolicy::Copy);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. Merge Resource settings
    // ─────────────────────────────────────────────────────────────────────────
    ApplyIfZero(result.resource.thread_count, defaults.resource.thread_count);
    ApplyIfZero(result.resource.max_queue_size, defaults.resource.max_queue_size);

    if (defaults.resource.execution_order.has_value()) {
        ApplyIfMatch(result.resource.execution_order, defaults.resource.execution_order,
                     ExecutionOrder::Sequential);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. Merge Face Analysis settings
    // ─────────────────────────────────────────────────────────────────────────
    ApplyIfZero(result.face_analysis.face_detector.score_threshold,
                defaults.face_analysis.score_threshold);
    ApplyIfZero(result.face_analysis.face_recognizer.similarity_threshold,
                defaults.face_analysis.similarity_threshold);

    // ─────────────────────────────────────────────────────────────────────────
    // 4. Apply hardcoded defaults for fields that are STILL empty/zero
    // (Ensure the final TaskConfig is fully usable)
    // ─────────────────────────────────────────────────────────────────────────
    if (result.io.output.video_encoder.empty()) result.io.output.video_encoder = "libx264";
    if (result.io.output.video_quality == 0) result.io.output.video_quality = 80;
    if (result.io.output.prefix.empty() && result.io.output.suffix.empty()) {
        result.io.output.prefix = "result_";
    }
    if (result.io.output.image_format.empty()) result.io.output.image_format = "png";
    if (result.resource.max_queue_size == 0) result.resource.max_queue_size = 20;
    if (result.face_analysis.face_detector.score_threshold == 0.0) {
        result.face_analysis.face_detector.score_threshold = 0.5;
    }
    if (result.face_analysis.face_landmarker.model.empty()) {
        result.face_analysis.face_landmarker.model = "2dfan4";
    }
    if (result.face_analysis.face_recognizer.model.empty()) {
        result.face_analysis.face_recognizer.model = "arcface_w600k_r50";
    }
    if (result.face_analysis.face_recognizer.similarity_threshold == 0.0) {
        result.face_analysis.face_recognizer.similarity_threshold = 0.6;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. Apply default model names to pipeline
    // ─────────────────────────────────────────────────────────────────────────
    ApplyDefaultModels(result, app.default_models);

    return result;
}

void ApplyDefaultModels(TaskConfig& task, const DefaultModels& defaults) {
    for (auto& step : task.pipeline) {
        if (step.step == "face_swapper") {
            if (auto* p = std::get_if<FaceSwapperParams>(&step.params)) {
                if (p->model.empty()) { p->model = defaults.face_swapper; }
            }
        } else if (step.step == "face_enhancer") {
            if (auto* p = std::get_if<FaceEnhancerParams>(&step.params)) {
                if (p->model.empty()) { p->model = defaults.face_enhancer; }
            }
        } else if (step.step == "frame_enhancer") {
            if (auto* p = std::get_if<FrameEnhancerParams>(&step.params)) {
                if (p->model.empty()) { p->model = defaults.frame_enhancer; }
            }
        } else if (step.step == "expression_restorer") {
            if (auto* p = std::get_if<ExpressionRestorerParams>(&step.params)) {
                if (p->model.empty()) {
                    // Expression restorer uses multiple models from DefaultModels,
                    // but the task config param only has one 'model' field for now.
                    // We'll set it to a representative name or handle it in the runner.
                    p->model = "live_portrait";
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
// ApplyCliParamsToStep: 将快捷模式 CLI 参数（cli_params 原始字符串 map）
// 转换为 step.params 的 typed variant（与 YAML 解析路径汇合）。
// 注意：不能 import config.parser（物理库依赖方向为 parser→core），
// 故枚举转换在此本地实现；CLI11 已预先校验合法值，此处为防御性处理。
// ─────────────────────────────────────────────────────────────────────────
namespace {

std::optional<FaceSelectorMode> parse_cli_selector_mode(const std::string& mode) {
    if (mode == "reference") return FaceSelectorMode::Reference;
    if (mode == "one") return FaceSelectorMode::One;
    if (mode == "many") return FaceSelectorMode::Many;
    return std::nullopt;
}

std::optional<double> parse_cli_double(const std::string& s) {
    if (s.empty()) return std::nullopt;
    try {
        return std::stod(s);
    } catch (const std::exception&) { return std::nullopt; }
}

} // namespace

Result<void, ConfigError> ApplyCliParamsToStep(PipelineStep& step) {
    const auto& cli = step.cli_params;
    if (cli.empty()) { return Result<void, ConfigError>::ok(); }

    auto get = [&cli](const std::string& key) -> std::string {
        auto it = cli.find(key);
        return it != cli.end() ? it->second : std::string{};
    };

    auto apply_selector_mode = [&get](const char* path, const std::string& mode_str,
                                      FaceSelectorMode& out) -> Result<void, ConfigError> {
        if (mode_str.empty()) { return Result<void, ConfigError>::ok(); }
        auto mode = parse_cli_selector_mode(mode_str);
        if (!mode) {
            return Result<void, ConfigError>::err(
                ConfigError(ErrorCode::E202ParameterOutOfRange,
                            "Invalid face_selector_mode: " + mode_str, path));
        }
        out = *mode;
        return Result<void, ConfigError>::ok();
    };

    if (step.step == "face_swapper") {
        FaceSwapperParams params;
        params.model = get("model");
        auto mode_r = apply_selector_mode("pipeline.step[face_swapper].face_selector_mode",
                                          get("face_selector_mode"), params.face_selector_mode);
        if (!mode_r) { return mode_r; }
        auto ref = get("reference_face_path");
        if (!ref.empty()) { params.reference_face_path = ref; }
        step.params = std::move(params);
    } else if (step.step == "face_enhancer") {
        FaceEnhancerParams params;
        params.model = get("model");
        auto factor = get("blend_factor");
        if (!factor.empty()) {
            auto v = parse_cli_double(factor);
            if (!v) {
                return Result<void, ConfigError>::err(ConfigError(
                    ErrorCode::E202ParameterOutOfRange, "Invalid blend_factor: " + factor,
                    "pipeline.step[face_enhancer].blend_factor"));
            }
            params.blend_factor = *v;
        }
        auto mode_r = apply_selector_mode("pipeline.step[face_enhancer].face_selector_mode",
                                          get("face_selector_mode"), params.face_selector_mode);
        if (!mode_r) { return mode_r; }
        auto ref = get("reference_face_path");
        if (!ref.empty()) { params.reference_face_path = ref; }
        step.params = std::move(params);
    } else if (step.step == "expression_restorer") {
        ExpressionRestorerParams params;
        params.model = get("model");
        auto factor = get("restore_factor");
        if (!factor.empty()) {
            auto v = parse_cli_double(factor);
            if (!v) {
                return Result<void, ConfigError>::err(ConfigError(
                    ErrorCode::E202ParameterOutOfRange, "Invalid restore_factor: " + factor,
                    "pipeline.step[expression_restorer].restore_factor"));
            }
            params.restore_factor = *v;
        }
        auto mode_r = apply_selector_mode("pipeline.step[expression_restorer].face_selector_mode",
                                          get("face_selector_mode"), params.face_selector_mode);
        if (!mode_r) { return mode_r; }
        auto ref = get("reference_face_path");
        if (!ref.empty()) { params.reference_face_path = ref; }
        step.params = std::move(params);
    } else if (step.step == "frame_enhancer") {
        FrameEnhancerParams params;
        params.model = get("model");
        auto factor = get("enhance_factor");
        if (!factor.empty()) {
            auto v = parse_cli_double(factor);
            if (!v) {
                return Result<void, ConfigError>::err(ConfigError(
                    ErrorCode::E202ParameterOutOfRange, "Invalid enhance_factor: " + factor,
                    "pipeline.step[frame_enhancer].enhance_factor"));
            }
            params.enhance_factor = *v;
        }
        step.params = std::move(params);
    } else {
        // Unknown step type: cannot apply typed params, leave untouched
        // (ConfigValidator rejects unknown processors before execution)
        return Result<void, ConfigError>::ok();
    }

    return Result<void, ConfigError>::ok();
}

} // namespace config
