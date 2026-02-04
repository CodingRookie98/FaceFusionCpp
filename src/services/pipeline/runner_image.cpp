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
