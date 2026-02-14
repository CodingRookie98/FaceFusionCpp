/**
 * @file runner_image.cpp
 * @brief Implementation of image processing logic for the pipeline runner
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>

export module services.pipeline.runner:image;

import domain.pipeline;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.logger;
import foundation.infrastructure.scoped_timer;
import :types;
import config.types;
import config.task; // Import TaskConfig

namespace services::pipeline {

using namespace domain::pipeline;
using namespace foundation::infrastructure::logger;
using namespace config; // Use config namespace for TaskConfig etc.

/**
 * @brief Helper class for image processing tasks
 */
export class ImageProcessingHelper {
public:
    /**
     * @brief Process a single image through the processing pipeline
     * @param target_path Path to the input image file
     * @param task_config Configuration for the task
     * @param progress_callback Optional callback for progress updates
     * @param context Processing context (models, sessions, etc.)
     * @param add_processors_func Function to populate the pipeline with processors
     * @return Result object indicating success or failure
     */
    static config::Result<void, config::ConfigError> ProcessImage(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<config::Result<void, config::ConfigError>(
            std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func) {
        using foundation::infrastructure::ScopedTimer;

        ScopedTimer timer("ImageProcessingHelper::ProcessImage",
                          std::format("target={}", target_path));

        cv::Mat image = cv::imread(target_path);
        if (image.empty()) {
            timer.set_result("error:load_failed");
            return config::Result<void, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E401ImageDecodeFailed, "Failed to load image: " + target_path));
        }

        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count = task_config.resource.get_effective_thread_count();
        pipeline_config.max_queue_size = task_config.resource.max_queue_size;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        ProcessorContext mutable_context = context;
        auto add_result = add_processors_func(pipeline, task_config, mutable_context);
        if (!add_result) {
            timer.set_result("error:add_processors_failed");
            return add_result;
        }
        pipeline->start();

        std::shared_ptr<const std::vector<float>> shared_source_embedding;
        if (!context.source_embedding.empty()) {
            shared_source_embedding =
                std::make_shared<const std::vector<float>>(context.source_embedding);
        }

        FrameData frame_data;
        frame_data.sequence_id = 0;
        frame_data.image = image;
        frame_data.source_embedding = shared_source_embedding;

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

        timer.set_result("success");
        return config::Result<void, config::ConfigError>::ok();
    }

    /**
     * @brief Process a batch of images through the processing pipeline
     * @param target_paths List of paths to input image files
     * @param task_config Configuration for the task
     * @param progress_callback Optional callback for progress updates
     * @param context Processing context (models, sessions, etc.)
     * @param add_processors_func Function to populate the pipeline with processors
     * @param cancelled Atomic flag to signal cancellation
     * @return Result object indicating success or failure
     */
    static config::Result<void, config::ConfigError> ProcessBatch(
        const std::vector<std::string>& target_paths, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<config::Result<void, config::ConfigError>(
            std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using foundation::infrastructure::ScopedTimer;

        ScopedTimer timer("ImageProcessingHelper::ProcessBatch",
                          std::format("count={}", target_paths.size()));

        if (target_paths.empty()) { return config::Result<void, config::ConfigError>::ok(); }

        // 1. Setup Pipeline
        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count = task_config.resource.get_effective_thread_count();
        pipeline_config.max_queue_size = task_config.resource.max_queue_size;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        ProcessorContext mutable_context = context;
        auto add_result = add_processors_func(pipeline, task_config, mutable_context);
        if (!add_result) {
            timer.set_result("error:add_processors_failed");
            return add_result;
        }
        pipeline->start();

        std::shared_ptr<const std::vector<float>> shared_source_embedding;
        if (!context.source_embedding.empty()) {
            shared_source_embedding =
                std::make_shared<const std::vector<float>>(context.source_embedding);
        }

        if (context.metrics_collector) {
            context.metrics_collector->set_total_frames(target_paths.size());
        }

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        // 2. Writer Thread
        std::thread writer_thread([&]() {
            int processed_count = 0;
            size_t total_images = target_paths.size();
            auto start_time = std::chrono::steady_clock::now();

            TaskProgress progress;
            progress.task_id = task_config.task_info.id;
            progress.total_frames = total_images;
            progress.current_step = "processing";

            while (true) {
                auto result_opt = pipeline->pop_frame();
                if (!result_opt) break;
                if (result_opt->is_end_of_stream) break;

                size_t index = result_opt->sequence_id;
                if (index >= target_paths.size()) {
                    writer_error = true;
                    writer_error_msg = "Invalid sequence ID received";
                    break;
                }

                auto output_path = GenerateOutputPath(target_paths[index], task_config);
                if (!cv::imwrite(output_path, result_opt->image)) {
                    writer_error = true;
                    writer_error_msg = "Failed to write output image: " + output_path;
                    if (context.metrics_collector) context.metrics_collector->record_frame_failed();
                    break;
                }

                if (context.metrics_collector) context.metrics_collector->record_frame_completed();

                processed_count++;
                if (progress_callback) { // Update progress every frame for images
                    auto now = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration<double>(now - start_time).count();
                    double fps =
                        (elapsed > 0.0) ? (static_cast<double>(processed_count) / elapsed) : 0.0;

                    progress.current_frame = processed_count;
                    progress.fps = fps;
                    progress_callback(progress);
                }
            }
        });

        // 3. Reader Loop
        size_t seq_id = 0;
        for (const auto& path : target_paths) {
            if (cancelled || writer_error) break;

            cv::Mat image = cv::imread(path);
            if (image.empty()) {
                // Log warning but continue? Or fail?
                // For batch processing, maybe skip and warn is better than failing entire batch.
                // But let's log error.
                Logger::get_instance()->warn("Failed to load image in batch: " + path);
                // We must ensure sequence_id alignment or handle missing frames.
                // If we skip pushing, the consumer won't get this sequence_id.
                // But consumer uses sequence_id to map to output path.
                // So if we skip, we just don't produce output for this one.
                // But we need to keep processed_count correct?
                // Let's just skip it.
                if (context.metrics_collector) context.metrics_collector->record_frame_failed();
                seq_id++; // Increment anyway to keep index sync? No, if we use seq_id as index into
                          // target_paths.
                // If we use seq_id as index, we MUST push something or the index will be wrong?
                // Wait, if I use `data.sequence_id = seq_id`, and `seq_id` iterates 0..N.
                // If I skip `imread`, and don't push, then `pop_frame` will never get that
                // `sequence_id`. That's fine, consumer just processes what it gets. BUT
                // `target_paths[index]` access depends on `sequence_id` being the index in
                // `target_paths`. So `seq_id` MUST match the loop index.
                continue;
            }

            FrameData data;
            data.sequence_id = seq_id;
            data.image = image;
            data.source_embedding = shared_source_embedding;
            pipeline->push_frame(std::move(data));

            seq_id++;
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) { writer_thread.join(); }

        pipeline->stop();

        if (cancelled) {
            timer.set_result("cancelled");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E407TaskCancelled, "Task cancelled"));
        }

        if (writer_error) {
            timer.set_result("error:writer_failed");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E406OutputWriteFailed, writer_error_msg));
        }

        timer.set_result("success");
        return config::Result<void, config::ConfigError>::ok();
    }

private:
    /**
     * @brief Generate output file path based on task configuration
     */
    static std::string GenerateOutputPath(const std::string& input_path,
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

} // namespace services::pipeline
