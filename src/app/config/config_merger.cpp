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

} // namespace config
