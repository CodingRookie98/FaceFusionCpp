/**
 * @file pipeline_runner.cpp
 * @brief PipelineRunner Implementation (Phase 3.1 Stable - Swapper Active)
 */
module;

#include <memory>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <variant>
#include <algorithm>
#include <opencv2/opencv.hpp>

module services.pipeline.runner;

import domain.pipeline;
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.frame.enhancer;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.masker;
import domain.face.analyser;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.media.ffmpeg;
import foundation.infrastructure.logger;
import foundation.infrastructure.scoped_timer;

import services.pipeline.processors.face_analysis;
import services.pipeline.metrics;
import :types;
import :video;
import :image;
import domain.pipeline.context;

namespace services::pipeline {

using namespace domain::pipeline;
using namespace foundation::ai::inference_session;
using namespace foundation::infrastructure::logger;

/**
 * @brief Decorator for IFrameProcessor to collect performance metrics
 */
class MetricsDecorator : public IFrameProcessor {
public:
    MetricsDecorator(std::shared_ptr<IFrameProcessor> processor, MetricsCollector* collector,
                     std::string step_name) :
        m_processor(std::move(processor)), m_collector(collector),
        m_step_name(std::move(step_name)) {}

    void process(FrameData& frame) override {
        if (m_collector) {
            ScopedStepTimer timer(*m_collector, m_step_name);
            m_processor->process(frame);
        } else {
            m_processor->process(frame);
        }
    }

    void ensure_loaded() override { m_processor->ensure_loaded(); }

private:
    std::shared_ptr<IFrameProcessor> m_processor;
    MetricsCollector* m_collector;
    std::string m_step_name;
};

// ProcessorContext defined in :types

// ============================================================================
// PipelineRunner::Impl
// ============================================================================

struct PipelineRunner::Impl {
    explicit Impl(const config::AppConfig& app_config) :
        m_app_config(app_config), m_running(false), m_cancelled(false) {
        m_model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        m_inference_options = Options::with_best_providers();

        // Ensure builtin adapters are registered
        domain::pipeline::RegisterBuiltinAdapters();
    }

    config::Result<void, config::ConfigError> Run(const config::TaskConfig& task_config,
                                                  ProgressCallback progress_callback) {
        using foundation::infrastructure::ScopedTimer;
        namespace logger = foundation::infrastructure::logger;

        ScopedTimer timer("PipelineRunner::Run",
                          std::format("task_id={}", task_config.task_info.id),
                          logger::LogLevel::Info);

        if (m_running.exchange(true)) {
            timer.set_result("error:already_running");
            return config::Result<void, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E400RuntimeError, "Pipeline is already running"));
        }
        m_cancelled = false;

        auto validate_result = config::validate_task_config(task_config);
        if (!validate_result) {
            m_running = false;
            timer.set_result("error:validation_failed");
            return config::Result<void, config::ConfigError>::err(validate_result.error());
        }

        auto result = ExecuteTask(task_config, progress_callback);

        // Export metrics if enabled
        if (m_metrics_collector && m_app_config.metrics.enable) {
            m_metrics_collector->export_json(m_app_config.metrics.report_path);
        }

        m_running = false;
        timer.set_result(result ? "success" : "error");
        return result;
    }

    void Cancel() { m_cancelled = true; }

    bool WaitForCompletion(std::chrono::seconds timeout) {
        Logger::get_instance()->info(
            "[PipelineRunner] Waiting for in-flight frames to complete...");

        auto deadline = std::chrono::steady_clock::now() + timeout;

        while (m_running.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        if (m_running.load()) {
            Logger::get_instance()->warn("[PipelineRunner] WaitForCompletion timed out");
            return false;
        }

        Logger::get_instance()->info("[PipelineRunner] All frames completed");
        return true;
    }

    bool IsRunning() const { return m_running; }

private:
    config::AppConfig m_app_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_cancelled;
    std::shared_ptr<domain::ai::model_repository::ModelRepository> m_model_repo;
    std::shared_ptr<domain::face::analyser::FaceAnalyser> m_face_analyser;
    Options m_inference_options;
    std::unique_ptr<MetricsCollector> m_metrics_collector;

    std::shared_ptr<domain::face::analyser::FaceAnalyser> GetFaceAnalyser() {
        if (!m_face_analyser) {
            domain::face::analyser::Options opts;
            opts.inference_session_options = m_inference_options;
            opts.model_paths.face_detector_yolo =
                m_model_repo->ensure_model(m_app_config.default_models.face_detector);
            opts.model_paths.face_recognizer_arcface =
                m_model_repo->ensure_model(m_app_config.default_models.face_recognizer);
            opts.face_detector_options.type = domain::face::detector::DetectorType::Yolo;
            opts.face_recognizer_type =
                domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50;

            m_face_analyser = std::make_shared<domain::face::analyser::FaceAnalyser>(opts);
        }
        return m_face_analyser;
    }

    config::Result<void, config::ConfigError> ExecuteTask(const config::TaskConfig& task_config,
                                                          ProgressCallback progress_callback) {
        if (task_config.io.target_paths.empty()) {
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E205RequiredFieldMissing,
                                    "No target paths specified", "io.target_paths"));
        }

        for (const auto& target_path : task_config.io.target_paths) {
            if (m_cancelled) break;

            auto result = ProcessTarget(target_path, task_config, progress_callback);
            if (!result) return result;
        }

        return config::Result<void, config::ConfigError>::ok();
    }

    config::Result<void, config::ConfigError> ProcessTarget(const std::string& target_path,
                                                            const config::TaskConfig& task_config,
                                                            ProgressCallback progress_callback) {
        namespace fs = std::filesystem;
        if (!fs::exists(target_path)) {
            return config::Result<void, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E402VideoOpenFailed, "Target file not found: " + target_path));
        }

        ProcessorContext context;
        context.model_repo = m_model_repo;
        context.inference_options = m_inference_options;
        context.face_analyser = GetFaceAnalyser();

        // Initialize Metrics if enabled
        if (m_app_config.metrics.enable) {
            m_metrics_collector = std::make_unique<MetricsCollector>(task_config.task_info.id);
            m_metrics_collector->set_gpu_sample_interval(
                std::chrono::milliseconds(m_app_config.metrics.gpu_sample_interval_ms));
            context.metrics_collector = m_metrics_collector.get();
        }

        if (!task_config.io.source_paths.empty()) {
            auto embed_result = LoadSourceEmbedding(task_config.io.source_paths[0]);
            if (embed_result) {
                context.source_embedding = std::move(embed_result).value();
            } else {
                return config::Result<void, config::ConfigError>::err(embed_result.error());
            }
        }

        bool is_video = foundation::media::ffmpeg::is_video(target_path);

        auto add_processors = [this](std::shared_ptr<Pipeline> p, const config::TaskConfig& c,
                                     ProcessorContext& ctx) {
            return this->AddProcessorsToPipeline(p, c, ctx);
        };

        if (is_video) {
            return VideoProcessingHelper::ProcessVideo(target_path, task_config, progress_callback,
                                                       context, add_processors, m_cancelled);
        } else {
            return ImageProcessingHelper::ProcessImage(target_path, task_config, progress_callback,
                                                       context, add_processors);
        }
    }

    config::Result<std::vector<float>, config::ConfigError> LoadSourceEmbedding(
        const std::string& source_path) {
        cv::Mat source_img = cv::imread(source_path);
        if (source_img.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E401ImageDecodeFailed,
                                    "Failed to load source image: " + source_path));
        }

        auto analyser = GetFaceAnalyser();
        if (!analyser) {
            return config::Result<std::vector<float>, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E100SystemError, "Failed to create FaceAnalyser"));
        }

        auto faces = analyser->get_many_faces(
            source_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);

        if (faces.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E403NoFaceDetected, "No face detected in source image"));
        }

        return config::Result<std::vector<float>, config::ConfigError>::ok(faces[0].embedding());
    }

    config::Result<void, config::ConfigError> AddProcessorsToPipeline(
        std::shared_ptr<Pipeline> pipeline, const config::TaskConfig& task_config,
        ProcessorContext& context) {
        services::pipeline::processors::FaceAnalysisRequirements reqs;
        bool needs_face_detection = false;

        // 1. Analyze Requirements and Initialize required services based on config
        domain::pipeline::PipelineContext domain_ctx;
        domain_ctx.inference_options = context.inference_options;
        domain_ctx.occluder = context.occluder;
        domain_ctx.region_masker = context.region_masker;

        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;

            if (step.step == "face_swapper") {
                needs_face_detection = true;
                reqs.need_swap_data = true;
                if (!domain_ctx.swapper) {
                    domain_ctx.swapper =
                        domain::face::swapper::FaceSwapperFactory::create_inswapper();

                    std::string model_name = "inswapper_128_fp16";
                    if (const auto* params = std::get_if<config::FaceSwapperParams>(&step.params)) {
                        if (!params->model.empty()) { model_name = params->model; }
                    }

                    auto model_path = m_model_repo->ensure_model(model_name);
                    if (model_path.empty()) {
                        // [E302] 模型缺失，立即中止 (design.md Section 5.3.1)
                        return config::Result<void, config::ConfigError>::err(
                            config::ConfigError(config::ErrorCode::E302ModelFileMissing,
                                                std::format("Model file not found: {}", model_name),
                                                "pipeline.step[face_swapper].model"));
                    } else {
                        domain_ctx.swapper->load_model(model_path, context.inference_options);
                        domain_ctx.swapper_model_path = model_path;
                    }
                }
            } else if (step.step == "face_enhancer") {
                needs_face_detection = true;
                reqs.need_enhance_data = true;
                if (!domain_ctx.face_enhancer) {
                    std::string model_name = "gfpgan_1.4";
                    if (const auto* params =
                            std::get_if<config::FaceEnhancerParams>(&step.params)) {
                        if (!params->model.empty()) { model_name = params->model; }
                    }

                    auto type = domain::face::enhancer::FaceEnhancerFactory::Type::GfpGan;
                    if (model_name.find("codeformer") != std::string::npos) {
                        type = domain::face::enhancer::FaceEnhancerFactory::Type::CodeFormer;
                    }

                    domain_ctx.face_enhancer =
                        domain::face::enhancer::FaceEnhancerFactory::create(type);

                    auto model_path = m_model_repo->ensure_model(model_name);
                    if (model_path.empty()) {
                        // [E302] 模型缺失，立即中止
                        return config::Result<void, config::ConfigError>::err(
                            config::ConfigError(config::ErrorCode::E302ModelFileMissing,
                                                std::format("Model file not found: {}", model_name),
                                                "pipeline.step[face_enhancer].model"));
                    } else {
                        domain_ctx.face_enhancer->load_model(model_path, context.inference_options);
                        domain_ctx.enhancer_model_path = model_path;
                    }
                }
            } else if (step.step == "expression_restorer") {
                needs_face_detection = true;
                reqs.need_expression_data = true;
                if (!domain_ctx.restorer) {
                    domain_ctx.restorer = domain::face::expression::create_live_portrait_restorer();

                    std::string model_name = "live_portrait";
                    if (const auto* params =
                            std::get_if<config::ExpressionRestorerParams>(&step.params)) {
                        if (!params->model.empty()) { model_name = params->model; }
                    }

                    // Live Portrait requires 3 models: feature, motion, generator
                    auto feature_path =
                        m_model_repo->ensure_model("live_portrait_feature_extractor");
                    auto motion_path = m_model_repo->ensure_model("live_portrait_motion_extractor");
                    auto gen_path = m_model_repo->ensure_model("live_portrait_generator");

                    if (feature_path.empty() || motion_path.empty() || gen_path.empty()) {
                        // [E302] 模型缺失，立即中止
                        return config::Result<void, config::ConfigError>::err(config::ConfigError(
                            config::ErrorCode::E302ModelFileMissing,
                            "Failed to find or download one of LivePortrait models",
                            "pipeline.step[expression_restorer].model"));
                    } else {
                        domain_ctx.restorer->load_model(feature_path, motion_path, gen_path,
                                                        context.inference_options);
                        domain_ctx.expression_feature_path = feature_path;
                        domain_ctx.expression_motion_path = motion_path;
                        domain_ctx.expression_generator_path = gen_path;
                    }
                }
            } else if (step.step == "frame_enhancer") {
                if (!domain_ctx.frame_enhancer_factory) {
                    std::string model_name = "real_esrgan_x4_plus";
                    if (const auto* params =
                            std::get_if<config::FrameEnhancerParams>(&step.params)) {
                        if (!params->model.empty()) { model_name = params->model; }
                    }

                    // Eagerly resolve model path
                    auto model_path = m_model_repo->ensure_model(model_name);
                    if (model_path.empty()) {
                        // [E302] 模型缺失，立即中止
                        return config::Result<void, config::ConfigError>::err(
                            config::ConfigError(config::ErrorCode::E302ModelFileMissing,
                                                std::format("Model file not found: {}", model_name),
                                                "pipeline.step[frame_enhancer].model"));
                    } else {
                        domain_ctx.frame_enhancer_model_path = model_path;
                    }

                    // Capture context.inference_options by value to avoid lifetime issues
                    auto options = context.inference_options;
                    // Capture resolved model_path instead of using m_model_repo inside lambda
                    domain_ctx.frame_enhancer_factory = [model_name, model_path, options]() {
                        auto type = domain::frame::enhancer::FrameEnhancerType::RealEsrGan;
                        if (model_name.find("hat") != std::string::npos) {
                            type = domain::frame::enhancer::FrameEnhancerType::RealHatGan;
                        }

                        // Factory handles loading internally.
                        // Note: Must pass model_name (key) not model_path, as factory derives scale
                        // from name.
                        return domain::frame::enhancer::FrameEnhancerFactory::create(
                            type, model_name, options);
                    };
                }
            }
        }

        // 2. Add Face Analysis Processor (if needed)
        if (needs_face_detection) {
            auto shared_emb = std::make_shared<const std::vector<float>>(context.source_embedding);
            pipeline->add_processor(
                std::make_shared<services::pipeline::processors::FaceAnalysisProcessor>(
                    context.face_analyser, shared_emb, reqs, context.metrics_collector));
        }

        // 3. Create Processors using Factory
        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;

            auto processor = ProcessorFactory::instance().create(step.step, &domain_ctx);
            if (processor) {
                if (context.metrics_collector) {
                    pipeline->add_processor(std::make_shared<MetricsDecorator>(
                        processor, context.metrics_collector, step.step));
                } else {
                    pipeline->add_processor(processor);
                }
            } else {
                Logger::get_instance()->warn("Failed to create processor for step: " + step.step);
            }
        }

        return config::Result<void, config::ConfigError>::ok();
    }
};

PipelineRunner::PipelineRunner(const config::AppConfig& app_config) :
    m_impl(std::make_unique<Impl>(app_config)) {}

PipelineRunner::~PipelineRunner() = default;

PipelineRunner::PipelineRunner(PipelineRunner&&) noexcept = default;
PipelineRunner& PipelineRunner::operator=(PipelineRunner&&) noexcept = default;

config::Result<void, config::ConfigError> PipelineRunner::Run(const config::TaskConfig& task_config,
                                                              ProgressCallback progress_callback) {
    return m_impl->Run(task_config, progress_callback);
}

void PipelineRunner::Cancel() {
    m_impl->Cancel();
}

bool PipelineRunner::WaitForCompletion(std::chrono::seconds timeout) {
    return m_impl->WaitForCompletion(timeout);
}

bool PipelineRunner::IsRunning() const {
    return m_impl->IsRunning();
}

std::unique_ptr<PipelineRunner> CreatePipelineRunner(const config::AppConfig& app_config) {
    return std::make_unique<PipelineRunner>(app_config);
}

} // namespace services::pipeline
