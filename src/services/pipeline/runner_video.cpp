/**
 * @file runner_video.cpp
 * @brief Implementation of video processing logic for the pipeline runner
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <filesystem>
#include <thread>
#include <iostream>
#include <opencv2/opencv.hpp>

export module services.pipeline.runner:video;

import domain.pipeline;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.media.ffmpeg;
import foundation.infrastructure.logger;
import :types;
import config.types;
import config.task; // Import TaskConfig

namespace services::pipeline {

using namespace domain::pipeline;
using namespace foundation::infrastructure::logger;
using namespace config; // Use config namespace for TaskConfig etc.

/**
 * @brief Helper class for video processing tasks
 */
export class VideoProcessingHelper {
public:
    /**
     * @brief Process a video file through the processing pipeline
     * @param target_path Path to the input video file
     * @param task_config Configuration for the task
     * @param progress_callback Optional callback for progress updates
     * @param context Processing context (models, sessions, etc.)
     * @param add_processors_func Function to populate the pipeline with processors
     * @param cancelled Atomic flag to signal cancellation
     * @return Result object indicating success or failure
     */
    static config::Result<void, config::ConfigError> ProcessVideo(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<void(std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using namespace foundation::media::ffmpeg;

        if (task_config.resource.memory_strategy == config::MemoryStrategy::Strict) {
            Logger::get_instance()->info("Running in Strict Mode with enhanced I/O optimization");
            return ProcessVideoStrict(target_path, task_config, progress_callback, context,
                                      add_processors_func, cancelled);
        }

        // 1. Open Reader
        VideoReader reader(target_path);
        if (!reader.open()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to open video: " + target_path));
        }

        // 2. Prepare Output
        std::string output_path = GenerateOutputPath(target_path, task_config);
        std::string video_output_path = output_path;
        bool needs_muxing = (task_config.io.output.audio_policy == config::AudioPolicy::Copy);

        if (needs_muxing) { video_output_path = output_path + ".temp.mp4"; }

        // 3. Open Writer
        VideoParams video_params;
        video_params.width = reader.get_width();
        video_params.height = reader.get_height();
        video_params.frameRate = reader.get_fps();

        VideoWriter writer(video_output_path, video_params);

        // 4. Setup Pipeline
        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;
        pipeline_config.max_queue_size = 8;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);

        // Mutable context for processor addition (if needed)
        ProcessorContext mutable_context = context;
        add_processors_func(pipeline, task_config, mutable_context);

        pipeline->start();

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        // 5. Writer Thread
        std::thread writer_thread([&]() {
            int frame_count = 0;
            int total_frames = reader.get_frame_count();

            while (true) {
                auto result_opt = pipeline->pop_frame();
                if (!result_opt) break;
                if (result_opt->is_end_of_stream) break;

                if (!writer.is_opened()) {
                    VideoParams actual_params = video_params;
                    actual_params.width = result_opt->image.cols;
                    actual_params.height = result_opt->image.rows;
                    writer = VideoWriter(video_output_path, actual_params);
                    if (!writer.open()) {
                        writer_error = true;
                        writer_error_msg = "Failed to open writer";
                        break;
                    }
                }

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

        // 6. Reader Loop
        long long seq_id = 0;
        cv::Mat frame;
        int max_frames = task_config.resource.max_frames;

        while (!cancelled && !writer_error) {
            if (max_frames > 0 && seq_id >= max_frames) break;

            frame = reader.read_frame();
            if (frame.empty()) break;

            FrameData data;
            data.sequence_id = seq_id++;
            data.image = frame;
            if (!context.source_embedding.empty()) {
                data.metadata["source_embedding"] = context.source_embedding;
            }
            pipeline->push_frame(std::move(data));
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) { writer_thread.join(); }

        pipeline->stop();
        writer.close();
        reader.close();

        if (cancelled) {
            std::filesystem::remove(video_output_path);
            if (needs_muxing && std::filesystem::exists(output_path))
                std::filesystem::remove(output_path);
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Task cancelled"));
        }

        if (writer_error) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError(writer_error_msg));
        }

        // 7. Muxing
        if (needs_muxing) {
            if (Remuxer::merge_av(video_output_path, target_path, output_path)) {
                std::filesystem::remove(video_output_path);
            } else {
                Logger::get_instance()->error("Failed to mux audio");
                if (std::filesystem::exists(output_path)) std::filesystem::remove(output_path);
                std::filesystem::rename(video_output_path, output_path);
            }
        }

        return config::Result<void, config::ConfigError>::Ok();
    }

private:
    /**
     * @brief Process video in strict mode (optimized for low memory usage)
     */
    static config::Result<void, config::ConfigError> ProcessVideoStrict(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<void(std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using namespace foundation::media::ffmpeg;

        VideoReader reader(target_path);
        if (!reader.open()) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Failed to open video"));
        }

        std::string output_path = GenerateOutputPath(target_path, task_config);
        std::string video_output_path = output_path;
        bool needs_muxing = (task_config.io.output.audio_policy == config::AudioPolicy::Copy);
        if (needs_muxing) { video_output_path = output_path + ".temp.mp4"; }

        VideoParams video_params;
        video_params.width = reader.get_width();
        video_params.height = reader.get_height();
        video_params.frameRate = reader.get_fps();

        VideoWriter writer(video_output_path, video_params);

        PipelineConfig pipeline_config;
        pipeline_config.max_queue_size = 4;
        pipeline_config.worker_thread_count =
            task_config.resource.thread_count > 0 ? task_config.resource.thread_count : 2;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        ProcessorContext mutable_context = context;
        add_processors_func(pipeline, task_config, mutable_context);
        pipeline->start();

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        std::thread writer_thread([&]() {
            int frame_count = 0;
            int total_frames = reader.get_frame_count();
            while (true) {
                auto result_opt = pipeline->pop_frame();
                if (!result_opt || result_opt->is_end_of_stream) break;

                if (!writer.is_opened()) {
                    VideoParams actual_params = video_params;
                    actual_params.width = result_opt->image.cols;
                    actual_params.height = result_opt->image.rows;
                    writer = VideoWriter(video_output_path, actual_params);
                    if (!writer.open()) {
                        writer_error = true;
                        writer_error_msg = "Failed to open writer";
                        break;
                    }
                }
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

        long long seq_id = 0;
        cv::Mat frame;
        while (!cancelled && !writer_error) {
            frame = reader.read_frame();
            if (frame.empty()) break;
            FrameData data;
            data.sequence_id = seq_id++;
            data.image = frame;
            if (!context.source_embedding.empty()) {
                data.metadata["source_embedding"] = context.source_embedding;
            }
            pipeline->push_frame(std::move(data));
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) writer_thread.join();
        pipeline->stop();
        writer.close();
        reader.close();

        if (cancelled) {
            std::filesystem::remove(video_output_path);
            if (needs_muxing && std::filesystem::exists(output_path))
                std::filesystem::remove(output_path);
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Task cancelled"));
        }
        if (writer_error)
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError(writer_error_msg));

        if (needs_muxing) {
            if (Remuxer::merge_av(video_output_path, target_path, output_path)) {
                std::filesystem::remove(video_output_path);
            } else {
                if (std::filesystem::exists(output_path)) std::filesystem::remove(output_path);
                std::filesystem::rename(video_output_path, output_path);
            }
        }
        return config::Result<void, config::ConfigError>::Ok();
    }

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
