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
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.media.ffmpeg;

namespace services::pipeline {

using namespace domain::pipeline;
using namespace foundation::ai::inference_session;

// ============================================================================
// ProcessorContext
// ============================================================================

struct ProcessorContext {
    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    std::vector<float> source_embedding;
    std::shared_ptr<domain::face::masker::IFaceOccluder> occluder;
    std::shared_ptr<domain::face::masker::IFaceRegionMasker> region_masker;
    Options inference_options;
};

// ============================================================================
// PipelineRunnerImpl
// ============================================================================

class PipelineRunnerImpl : public IPipelineRunner {
public:
    explicit PipelineRunnerImpl(const config::AppConfig& app_config) :
        m_app_config(app_config), m_running(false), m_cancelled(false) {
        m_model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        m_inference_options = Options::with_best_providers();
    }

    config::Result<void, config::ConfigError> Run(const config::TaskConfig& task_config,
                                                  ProgressCallback progress_callback) override {
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

    void Cancel() override { m_cancelled = true; }
    bool IsRunning() const override { return m_running; }

private:
    config::AppConfig m_app_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_cancelled;
    std::shared_ptr<domain::ai::model_repository::ModelRepository> m_model_repo;
    Options m_inference_options;

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

        auto ext = fs::path(target_path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool is_video =
            (ext == ".mp4" || ext == ".avi" || ext == ".mkv" || ext == ".mov" || ext == ".webm");

        return is_video ? ProcessVideo(target_path, task_config, progress_callback) :
                          ProcessImage(target_path, task_config, progress_callback);
    }

    config::Result<void, config::ConfigError> ProcessImage(const std::string& target_path,
                                                           const config::TaskConfig& task_config,
                                                           ProgressCallback progress_callback) {
        cv::Mat image = cv::imread(target_path);
        if (image.empty()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to load image: " + target_path));
        }

        ProcessorContext context;
        context.model_repo = m_model_repo;
        context.inference_options = m_inference_options;

        if (!task_config.io.source_paths.empty()) {
            auto embed_result = LoadSourceEmbedding(task_config.io.source_paths[0]);
            if (embed_result) {
                context.source_embedding = std::move(embed_result).value();
            } else {
                return config::Result<void, config::ConfigError>::Err(embed_result.error());
            }
        }

        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;
        pipeline_config.max_queue_size = 16;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        AddProcessorsToPipeline(pipeline, task_config, context);
        pipeline->start();

        FrameData frame_data;
        frame_data.sequence_id = 0;
        frame_data.image = image;
        if (!context.source_embedding.empty()) {
            frame_data.metadata["source_embedding"] = context.source_embedding;
        }

        pipeline->push_frame(std::move(frame_data));

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = 1;
        pipeline->push_frame(std::move(eos));

        auto result_opt = pipeline->pop_frame();
        if (result_opt && !result_opt->is_end_of_stream) {
            auto output_path = GenerateOutputPath(target_path, task_config);
            cv::imwrite(output_path, result_opt->image);
        }

        if (progress_callback) {
            TaskProgress progress;
            progress.task_id = task_config.task_info.id;
            progress.current_frame = 1;
            progress.total_frames = 1;
            progress.current_step = "completed";
            progress_callback(progress);
        }

        return config::Result<void, config::ConfigError>::Ok();
    }

    config::Result<void, config::ConfigError> ProcessVideo(const std::string& target_path,
                                                           const config::TaskConfig& task_config,
                                                           ProgressCallback progress_callback) {
        using namespace foundation::media::ffmpeg;

        if (task_config.resource.memory_strategy == config::MemoryStrategy::Strict) {
            return ProcessVideoStrict(target_path, task_config, progress_callback);
        }

        if (task_config.resource.memory_strategy == config::MemoryStrategy::Strict) {
            return ProcessVideoStrict(target_path, task_config, progress_callback);
        }

        // 1. Open Reader
        VideoReader reader(target_path);
        if (!reader.open()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to open video: " + target_path));
        }

        // 2. Prepare Output Path
        std::string output_path = GenerateOutputPath(target_path, task_config);

        // 3. Open Writer
        VideoParams video_params;
        video_params.width = reader.get_width();
        video_params.height = reader.get_height();
        video_params.frameRate = reader.get_fps();

        VideoWriter writer(output_path, video_params);
        if (!writer.open()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to open video writer: " + output_path));
        }

        // 4. Prepare Context
        ProcessorContext context;
        context.model_repo = m_model_repo;
        context.inference_options = m_inference_options;

        if (!task_config.io.source_paths.empty()) {
            auto embed_result = LoadSourceEmbedding(task_config.io.source_paths[0]);
            if (embed_result) {
                context.source_embedding = std::move(embed_result).value();
            } else {
                return config::Result<void, config::ConfigError>::Err(embed_result.error());
            }
        }

        // 5. Setup Pipeline
        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;
        pipeline_config.max_queue_size = 8;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        AddProcessorsToPipeline(pipeline, task_config, context);
        pipeline->start();

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        // 6. Start Writer Thread (Sink)
        std::thread writer_thread([&]() {
            int frame_count = 0;
            int total_frames = reader.get_frame_count();

            while (true) {
                auto result_opt = pipeline->pop_frame();
                if (!result_opt) break; // Should not happen unless pipeline stopped abruptly

                if (result_opt->is_end_of_stream) break;

                if (!writer.write_frame(result_opt->image)) {
                    writer_error = true;
                    writer_error_msg = "Failed to write frame";
                    break;
                }

                frame_count++;
                if (progress_callback && frame_count % 10 == 0) {
                    TaskProgress progress;
                    progress.task_id = task_config.task_info.id;
                    progress.current_frame = frame_count;
                    progress.total_frames = total_frames;
                    progress.current_step = "processing";
                    progress_callback(progress);
                }
            }
        });

        // 7. Main Thread: Read & Push (Source)
        long long seq_id = 0;
        cv::Mat frame;
        while (!m_cancelled && !writer_error) {
            frame = reader.read_frame();
            if (frame.empty()) break; // EOF

            FrameData data;
            data.sequence_id = seq_id++;
            data.image = frame;
            if (!context.source_embedding.empty()) {
                data.metadata["source_embedding"] = context.source_embedding;
            }

            // push_frame blocks if full
            pipeline->push_frame(std::move(data));
        }

        // Push EOS
        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        // Wait for writer to finish
        if (writer_thread.joinable()) { writer_thread.join(); }

        pipeline->stop();
        writer.close();
        reader.close();

        if (m_cancelled) {
            std::filesystem::remove(output_path);
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Task cancelled"));
        }

        if (writer_error) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError(writer_error_msg));
        }

        // 8. Handle Audio
        // 8. Handle Audio
        if (task_config.io.output.audio_policy == config::AudioPolicy::Copy) {
            writer.set_audio_source(target_path);
        }

        return config::Result<void, config::ConfigError>::Ok();
    }

    config::Result<void, config::ConfigError> ProcessVideoStrict(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback) {
        using namespace foundation::media::ffmpeg;
        namespace fs = std::filesystem;

        std::vector<config::PipelineStep> enabled_steps;
        for (const auto& step : task_config.pipeline) {
            if (step.enabled) enabled_steps.push_back(step);
        }

        if (enabled_steps.empty()) return config::Result<void, config::ConfigError>::Ok();

        ProcessorContext context;
        context.model_repo = m_model_repo;
        context.inference_options = m_inference_options;
        if (!task_config.io.source_paths.empty()) {
            auto embed_result = LoadSourceEmbedding(task_config.io.source_paths[0]);
            if (embed_result) {
                context.source_embedding = std::move(embed_result).value();
            } else {
                return config::Result<void, config::ConfigError>::Err(embed_result.error());
            }
        }

        std::string current_input_path = target_path;
        std::string final_output_path = GenerateOutputPath(target_path, task_config);
        std::string temp_output_path;

        bool is_first_step = true;

        for (size_t i = 0; i < enabled_steps.size(); ++i) {
            if (m_cancelled) break;

            const auto& step = enabled_steps[i];
            bool is_last_step = (i == enabled_steps.size() - 1);

            if (is_last_step) {
                temp_output_path = final_output_path;
            } else {
                temp_output_path = (fs::path(task_config.io.output.path)
                                    / ("temp_step_" + std::to_string(i) + ".mp4"))
                                       .string();
            }

            auto result = ProcessVideoSingleStep(current_input_path, temp_output_path, step,
                                                 task_config, context, progress_callback);
            if (!result) return result;

            if (!is_first_step) { fs::remove(current_input_path); }

            current_input_path = temp_output_path;
            is_first_step = false;
        }

        if (m_cancelled) {
            if (fs::exists(final_output_path)) fs::remove(final_output_path);
            if (!is_first_step && fs::exists(current_input_path)) fs::remove(current_input_path);
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Task cancelled"));
        }

        if (task_config.io.output.audio_policy == config::AudioPolicy::Copy) {
            // Audio copy implementation omitted for Strict mode currently.
        }

        return config::Result<void, config::ConfigError>::Ok();
    }

    config::Result<void, config::ConfigError> ProcessVideoSingleStep(
        const std::string& input_path, const std::string& output_path,
        const config::PipelineStep& step, const config::TaskConfig& task_config,
        ProcessorContext& context, ProgressCallback progress_callback) {
        using namespace foundation::media::ffmpeg;

        VideoReader reader(input_path);
        if (!reader.open())
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed open: " + input_path));

        VideoParams params;
        params.width = reader.get_width();
        params.height = reader.get_height();
        params.frameRate = reader.get_fps();

        VideoWriter writer(output_path, params);
        if (!writer.open())
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed open writer: " + output_path));

        PipelineConfig p_config;
        p_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;
        p_config.max_queue_size = 4;

        auto pipeline = std::make_shared<Pipeline>(p_config);

        bool needs_det = (step.step == "face_swapper" || step.step == "face_enhancer"
                          || step.step == "expression_restorer");
        if (needs_det) {
            auto det = CreateDetectorProcessor(context);
            if (det) pipeline->add_processor(det);
        }
        auto proc = CreateProcessorFromStep(step, context);
        if (proc) pipeline->add_processor(proc);

        pipeline->start();

        std::atomic<bool> writer_error = false;
        std::thread writer_thread([&]() {
            while (true) {
                auto res = pipeline->pop_frame();
                if (!res || res->is_end_of_stream) break;
                if (!writer.write_frame(res->image)) {
                    writer_error = true;
                    break;
                }
            }
        });

        long long seq = 0;
        cv::Mat frame;
        while (!m_cancelled && !writer_error) {
            frame = reader.read_frame();
            if (frame.empty()) break;
            FrameData data;
            data.sequence_id = seq++;
            data.image = frame;
            if (!context.source_embedding.empty())
                data.metadata["source_embedding"] = context.source_embedding;
            pipeline->push_frame(std::move(data));
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) writer_thread.join();

        pipeline->stop();
        writer.close();
        reader.close();

        if (writer_error)
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Writer error"));

        return config::Result<void, config::ConfigError>::Ok();
    }

    config::Result<std::vector<float>, config::ConfigError> LoadSourceEmbedding(
        const std::string& source_path) {
        cv::Mat source_img = cv::imread(source_path);
        if (source_img.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("Failed to load source image: " + source_path));
        }

        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::Yolo);
        std::string det_model = m_model_repo->ensure_model("face_detector_yoloface");
        if (det_model.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("Face detector model not found"));
        }
        detector->load_model(det_model, m_inference_options);
        auto faces = detector->detect(source_img);

        if (faces.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("No face detected in source image"));
        }

        auto recognizer = domain::face::recognizer::create_face_recognizer(
            domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);
        std::string rec_model = m_model_repo->ensure_model("face_recognizer_arcface_w600k_r50");
        if (rec_model.empty()) {
            return config::Result<std::vector<float>, config::ConfigError>::Err(
                config::ConfigError("Face recognizer model not found"));
        }
        recognizer->load_model(rec_model, m_inference_options);

        auto [normed_emb, raw_emb] = recognizer->recognize(source_img, faces[0].landmarks);
        return config::Result<std::vector<float>, config::ConfigError>::Ok(std::move(raw_emb));
    }

    void AddProcessorsToPipeline(std::shared_ptr<Pipeline> pipeline,
                                 const config::TaskConfig& task_config, ProcessorContext& context) {
        bool needs_face_detection = false;
        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;
            if (step.step == "face_swapper" || step.step == "face_enhancer"
                || step.step == "expression_restorer") {
                needs_face_detection = true;
                break;
            }
        }

        if (needs_face_detection) {
            auto detector_processor = CreateDetectorProcessor(context);
            if (detector_processor) { pipeline->add_processor(detector_processor); }
        }

        for (const auto& step : task_config.pipeline) {
            if (!step.enabled) continue;
            auto processor = CreateProcessorFromStep(step, context);
            if (processor) { pipeline->add_processor(processor); }
        }
    }

    std::shared_ptr<IFrameProcessor> CreateDetectorProcessor(const ProcessorContext& context) {
        class DetectorProcessor : public IFrameProcessor {
        public:
            DetectorProcessor(std::shared_ptr<domain::face::detector::IFaceDetector> det,
                              std::vector<float> src_emb) :
                detector(std::move(det)), source_embedding(std::move(src_emb)) {}
            void process(FrameData& frame) override {
                auto results = detector->detect(frame.image);
                if (results.empty()) return;
                domain::face::swapper::SwapInput swap_input;
                swap_input.target_frame = frame.image;
                for (const auto& face : results)
                    swap_input.target_faces_landmarks.push_back(face.landmarks);
                swap_input.source_embedding = source_embedding;
                frame.metadata["swap_input"] = swap_input;
                domain::face::enhancer::EnhanceInput enhance_input;
                enhance_input.target_frame = frame.image;
                for (const auto& face : results)
                    enhance_input.target_faces_landmarks.push_back(face.landmarks);
                enhance_input.face_blend = 80;
                frame.metadata["enhance_input"] = enhance_input;
            }

        private:
            std::shared_ptr<domain::face::detector::IFaceDetector> detector;
            std::vector<float> source_embedding;
        };

        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::Yolo);
        std::string model = context.model_repo->ensure_model("face_detector_yoloface");
        if (model.empty()) return nullptr;
        detector->load_model(model, context.inference_options);
        return std::make_shared<DetectorProcessor>(std::move(detector), context.source_embedding);
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
        std::string model = context.model_repo->ensure_model(
            params->model.empty() ? "inswapper_128" : params->model);
        if (model.empty()) return nullptr;

        swapper->load_model(model, context.inference_options);

        return std::shared_ptr<IFrameProcessor>(new domain::pipeline::SwapperAdapter(
            std::move(swapper), context.occluder, context.region_masker));
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
        std::string model_name =
            "face_enhancer_" + (params->model.empty() ? "gfpgan_1.4" : params->model);
        std::string model = context.model_repo->ensure_model(model_name);
        if (model.empty()) return nullptr;

        enhancer_ptr->load_model(model, context.inference_options);

        // Explicitly move into a shared_ptr of the interface type first to ensure correct type
        // helper usage
        std::shared_ptr<domain::face::enhancer::IFaceEnhancer> shared_enhancer =
            std::move(enhancer_ptr);

        return std::shared_ptr<IFrameProcessor>(new domain::pipeline::FaceEnhancerAdapter(
            shared_enhancer, context.occluder, context.region_masker));
    }

    std::shared_ptr<IFrameProcessor> CreateExpressionProcessor(const config::PipelineStep& step,
                                                               const ProcessorContext& context) {
        auto* params = std::get_if<config::ExpressionRestorerParams>(&step.params);
        if (!params) return nullptr;

        auto restorer_ptr = domain::face::expression::create_live_portrait_restorer();

        // LivePortrait requires 3 discrete models
        std::string feature_path =
            context.model_repo->ensure_model("live_portrait_feature_extractor");
        std::string motion_path =
            context.model_repo->ensure_model("live_portrait_motion_extractor");
        std::string generator_path = context.model_repo->ensure_model("live_portrait_generator");

        if (feature_path.empty() || motion_path.empty() || generator_path.empty()) {
            return nullptr;
        }

        restorer_ptr->load_model(feature_path, motion_path, generator_path,
                                 context.inference_options);

        std::shared_ptr<domain::face::expression::IFaceExpressionRestorer> shared_restorer =
            std::move(restorer_ptr);

        return std::shared_ptr<IFrameProcessor>(
            new domain::pipeline::ExpressionAdapter(shared_restorer));
    }

    std::shared_ptr<IFrameProcessor> CreateFrameEnhancerProcessor(const config::PipelineStep& step,
                                                                  const ProcessorContext& context) {
        auto* params = std::get_if<config::FrameEnhancerParams>(&step.params);
        if (!params) return nullptr;

        using Type = domain::frame::enhancer::FrameEnhancerType;
        Type type = Type::RealEsrGan;
        if (params->model.find("real_hat") != std::string::npos) { type = Type::RealHatGan; }

        std::string model_name =
            "frame_enhancer_" + (params->model.empty() ? "real_esrgan_x4plus" : params->model);
        std::string model_path = context.model_repo->ensure_model(model_name);
        if (model_path.empty()) return nullptr;

        auto enhancer_ptr = domain::frame::enhancer::FrameEnhancerFactory::create(
            type, model_path, context.inference_options);

        if (!enhancer_ptr) return nullptr;

        return std::shared_ptr<IFrameProcessor>(
            new domain::pipeline::FrameEnhancerAdapter(std::move(enhancer_ptr)));
    }

    std::string GenerateOutputPath(const std::string& input_path,
                                   const config::TaskConfig& task_config) {
        namespace fs = std::filesystem;
        fs::path input(input_path);
        fs::path output_dir = task_config.io.output.path;
        if (!fs::exists(output_dir)) fs::create_directories(output_dir);

        std::string filename =
            task_config.io.output.prefix + input.stem().string() + task_config.io.output.suffix;
        std::string ext = input.extension().string();
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
            ext = "." + task_config.io.output.image_format;
        }
        return (output_dir / (filename + ext)).string();
    }
};

std::unique_ptr<IPipelineRunner> CreatePipelineRunner(const config::AppConfig& app_config) {
    return std::make_unique<PipelineRunnerImpl>(app_config);
}

} // namespace services::pipeline
