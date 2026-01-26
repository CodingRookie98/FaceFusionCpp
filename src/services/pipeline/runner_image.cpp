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
import :types;
import config.types;
import config.task; // Import TaskConfig

namespace services::pipeline {

using namespace domain::pipeline;
using namespace config; // Use config namespace for TaskConfig etc.

export class ImageProcessingHelper {
public:
    static config::Result<void, config::ConfigError> ProcessImage(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<void(std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func) {
        cv::Mat image = cv::imread(target_path);
        if (image.empty()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to load image: " + target_path));
        }

        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;
        pipeline_config.max_queue_size = 16;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        ProcessorContext mutable_context = context;
        add_processors_func(pipeline, task_config, mutable_context);
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

private:
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
