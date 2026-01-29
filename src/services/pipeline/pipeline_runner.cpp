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

import services.pipeline.processors.face_analysis;
import :types;
import :video;
import :image;

namespace services::pipeline {

using namespace domain::pipeline;
using namespace foundation::ai::inference_session;
using namespace foundation::infrastructure::logger;

// ProcessorContext defined in :types

// ============================================================================
// PipelineRunner::Impl
// ============================================================================

struct PipelineRunner::Impl {
    explicit Impl(const config::AppConfig& app_config) :
        m_app_config(app_config), m_running(false), m_cancelled(false) {
        m_model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        m_inference_options = Options::with_best_providers();
    }

    config::Result<void, config::ConfigError> Run(const config::TaskConfig& task_config,
                                                  ProgressCallback progress_callback) {
        if (m_running.exchange(true)) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Pipeline is already running"));
        }
        m_cancelled = false;

        auto validate_result = config::ValidateTaskConfig(task_config);
        if (!validate_result) {
            m_running = false;
            return config::Result<void, config::ConfigError>::Err(validate_result.error());
        }

        auto result = ExecuteTask(task_config, progress_callback);

        m_running = false;
        return result;
    }

    void Cancel() { m_cancelled = true; }
    bool IsRunning() const { return m_running; }

private:
    config::AppConfig m_app_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_cancelled;
    std::shared_ptr<domain::ai::model_repository::ModelRepository> m_model_repo;
    std::shared_ptr<domain::face::analyser::FaceAnalyser> m_face_analyser;
    Options m_inference_options;

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
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("No target paths specified", "io.target_paths"));
        }

        for (const auto& target_path : task_config.io.target_paths) {
            if (m_cancelled) break;

            auto result = ProcessTarget(target_path, task_config, progress_callback);
            if (!result) return result;
        }

        return config::Result<void, config::ConfigError>::Ok();
    }

    config::Result<void, config::ConfigError> ProcessTarget(const std::string& target_path,
                                                            const config::TaskConfig& task_config,
                                                            ProgressCallback progress_callback) {
        namespace fs = std::filesystem;
        if (!fs::exists(target_path)) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Target file not found: " + target_path));
        }

        ProcessorContext context;
        context.model_repo = m_model_repo;
        context.inference_options = m_inference_options;
        context.face_analyser = GetFaceAnalyser();

        if (!task_config.io.source_paths.empty()) {
            auto embed_result = LoadSourceEmbedding(task_config.io.source_paths[0]);
            if (embed_result) {
                context.source_embedding = std::move(embed_result).value();
            } else {
                return config::Result<void, config::ConfigError>::Err(embed_result.error());
            }
        }

        bool is_video = foundation::media::ffmpeg::is_video(target_path);

        auto add_processors = [this](std::shared_ptr<Pipeline> p, const config::TaskConfig& c,
                                     ProcessorContext& ctx) {
            this->AddProcessorsToPipeline(p, c, ctx);
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
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("Failed to load source image: " + source_path));
        }

        auto analyser = GetFaceAnalyser();
        if (!analyser) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("Failed to create FaceAnalyser"));
        }

        auto faces = analyser->get_many_faces(
            source_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);

        if (faces.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("No face detected in source image"));
        }

        return config::Result<std::vector<float>, config::ConfigError>::Ok(faces[0].embedding());
    }

    void AddProcessorsToPipeline(std::shared_ptr<Pipeline> pipeline,
                                 const config::TaskConfig& task_config, ProcessorContext& context) {
        services::pipeline::processors::FaceAnalysisRequirements reqs;
        bool needs_face_detection = false;

        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;
            if (step.step == "face_swapper") {
                needs_face_detection = true;
                reqs.need_swap_data = true;
            } else if (step.step == "face_enhancer") {
                needs_face_detection = true;
                reqs.need_enhance_data = true;
            } else if (step.step == "expression_restorer") {
                needs_face_detection = true;
                reqs.need_expression_data = true;
            }
        }

        if (needs_face_detection) {
            auto detector_processor = CreateFaceAnalysisProcessor(context, reqs);
            if (detector_processor) { pipeline->add_processor(detector_processor); }
        }

        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;
            auto processor = CreateProcessorFromStep(step, context);
            if (processor) { pipeline->add_processor(processor); }
        }
    }

    std::shared_ptr<IFrameProcessor> CreateFaceAnalysisProcessor(
        const ProcessorContext& context,
        services::pipeline::processors::FaceAnalysisRequirements reqs) {
        if (!context.face_analyser) return nullptr;
        return std::make_shared<services::pipeline::processors::FaceAnalysisProcessor>(
            context.face_analyser, context.source_embedding, reqs);
    }

    std::shared_ptr<IFrameProcessor> CreateProcessorFromStep(const config::PipelineStep& step,
                                                             const ProcessorContext& context) {
        if (step.step == "face_swapper") return CreateFaceSwapperProcessor(step, context);
        if (step.step == "face_enhancer") return CreateFaceEnhancerProcessor(step, context);
        if (step.step == "expression_restorer") return CreateExpressionProcessor(step, context);
        if (step.step == "frame_enhancer") return CreateFrameEnhancerProcessor(step, context);
        return nullptr;
    }

    std::shared_ptr<IFrameProcessor> CreateFaceSwapperProcessor(const config::PipelineStep& step,
                                                                const ProcessorContext& context) {
        auto* params = std::get_if<config::FaceSwapperParams>(&step.params);
        if (!params) return nullptr;

        auto swapper = domain::face::swapper::FaceSwapperFactory::create_inswapper();

        std::string model_name =
            params->model.empty() ? m_app_config.default_models.face_swapper : params->model;
        std::string model = context.model_repo->ensure_model(model_name);
        if (model.empty()) return nullptr;

        return std::make_shared<domain::pipeline::SwapperAdapter>(
            std::move(swapper), model, context.inference_options, context.occluder,
            context.region_masker);
    }

    std::shared_ptr<IFrameProcessor> CreateFaceEnhancerProcessor(const config::PipelineStep& step,
                                                                 const ProcessorContext& context) {
        auto* params = std::get_if<config::FaceEnhancerParams>(&step.params);
        if (!params) return nullptr;

        domain::face::enhancer::FaceEnhancerFactory::Type type =
            domain::face::enhancer::FaceEnhancerFactory::Type::GfpGan;

        if (params->model.find("codeformer") != std::string::npos) {
            type = domain::face::enhancer::FaceEnhancerFactory::Type::CodeFormer;
        }

        auto enhancer_ptr = domain::face::enhancer::FaceEnhancerFactory::create(type);

        std::string model_base_name =
            params->model.empty() ? m_app_config.default_models.face_enhancer : params->model;
        std::string model_name = model_base_name;

        std::string model = context.model_repo->ensure_model(model_name);
        if (model.empty()) return nullptr;

        std::shared_ptr<domain::face::enhancer::IFaceEnhancer> shared_enhancer =
            std::move(enhancer_ptr);

        return std::make_shared<domain::pipeline::FaceEnhancerAdapter>(
            shared_enhancer, model, context.inference_options, context.occluder,
            context.region_masker);
    }

    std::shared_ptr<IFrameProcessor> CreateExpressionProcessor(const config::PipelineStep& step,
                                                               const ProcessorContext& context) {
        auto* params = std::get_if<config::ExpressionRestorerParams>(&step.params);
        if (!params) return nullptr;

        auto restorer_ptr = domain::face::expression::create_live_portrait_restorer();

        std::string feature_path = context.model_repo->ensure_model(
            m_app_config.default_models.expression_restorer_feature);
        std::string motion_path = context.model_repo->ensure_model(
            m_app_config.default_models.expression_restorer_motion);
        std::string generator_path = context.model_repo->ensure_model(
            m_app_config.default_models.expression_restorer_generator);

        if (feature_path.empty() || motion_path.empty() || generator_path.empty()) {
            return nullptr;
        }

        std::shared_ptr<domain::face::expression::IFaceExpressionRestorer> shared_restorer =
            std::move(restorer_ptr);

        return std::make_shared<domain::pipeline::ExpressionAdapter>(
            shared_restorer, feature_path, motion_path, generator_path, context.inference_options);
    }

    std::shared_ptr<IFrameProcessor> CreateFrameEnhancerProcessor(const config::PipelineStep& step,
                                                                  const ProcessorContext& context) {
        auto* params = std::get_if<config::FrameEnhancerParams>(&step.params);
        if (!params) return nullptr;

        using Type = domain::frame::enhancer::FrameEnhancerType;
        Type type = Type::RealEsrGan;

        if (params->model.find("real_hat") != std::string::npos) { type = Type::RealHatGan; }

        std::string model_base_name =
            params->model.empty() ? m_app_config.default_models.frame_enhancer : params->model;
        std::string model_name = model_base_name;

        std::string model_path = context.model_repo->ensure_model(model_name);
        if (model_path.empty()) return nullptr;

        auto factory = [type, model_name, options = context.inference_options]() {
            return domain::frame::enhancer::FrameEnhancerFactory::create(type, model_name, options);
        };

        return std::make_shared<domain::pipeline::FrameEnhancerAdapter>(std::move(factory));
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

bool PipelineRunner::IsRunning() const {
    return m_impl->IsRunning();
}

std::unique_ptr<PipelineRunner> CreatePipelineRunner(const config::AppConfig& app_config) {
    return std::make_unique<PipelineRunner>(app_config);
}

} // namespace services::pipeline
