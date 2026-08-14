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
#include <chrono>
#include <iostream>
#include <cmath>
#include <format>
#include <opencv2/opencv.hpp>

export module services.pipeline.runner:video;

import domain.pipeline;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.media.ffmpeg;
import foundation.infrastructure.logger;
import foundation.infrastructure.scoped_timer;
import foundation.infrastructure.crypto;
import foundation.infrastructure.cuda_utils;
import services.pipeline.checkpoint;
import services.pipeline.metrics;
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
        std::function<config::Result<void, config::ConfigError>(
            std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using namespace foundation::media::ffmpeg;
        using foundation::infrastructure::ScopedTimer;

        ScopedTimer timer("VideoProcessingHelper::ProcessVideo",
                          std::format("target={}", target_path));

        // Video segmentation: process the video in time-based segments when configured
        if (task_config.resource.segment_duration_seconds > 0) {
            if (task_config.task_info.enable_resume) {
                Logger::get_instance()->warn(
                    "[VideoRunner] enable_resume with segment_duration_seconds is not supported; "
                    "falling back to whole-video processing without resume");
                // Fall through to normal processing below (resume disabled)
            } else {
                return ProcessVideoSegmented(target_path, task_config, progress_callback, context,
                                             add_processors_func, cancelled);
            }
        }

        if (task_config.resource.memory_strategy == config::MemoryStrategy::Strict) {
            Logger::get_instance()->info("Running in Strict Mode with enhanced I/O optimization");
            return ProcessVideoStrict(target_path, task_config, progress_callback, context,
                                      add_processors_func, cancelled);
        }

        // 1. Open Reader
        VideoReader reader(target_path);
        if (!reader.open()) {
            auto err = config::ConfigError(config::ErrorCode::E402VideoOpenFailed,
                                           std::format("Failed to open video: {}", target_path),
                                           "io.target_paths");
            timer.set_result("error:open_failed");
            return config::Result<void, config::ConfigError>::err(err);
        }

        int64_t start_frame = 0;
        int64_t total_frames = reader.get_frame_count();
        std::unique_ptr<CheckpointManager> ckpt_mgr;
        std::string config_hash;

        // Try to resume if enabled
        if (task_config.task_info.enable_resume) {
            ckpt_mgr = std::make_unique<CheckpointManager>("./checkpoints");
            config_hash = CalculateConfigHash(task_config);

            if (auto saved = ckpt_mgr->load(task_config.task_info.id, config_hash)) {
                start_frame = saved->last_completed_frame + 1;

                if (start_frame >= total_frames) {
                    Logger::get_instance()->info(
                        "[VideoRunner] Task already completed, nothing to resume");
                    ckpt_mgr->cleanup(task_config.task_info.id);
                    return config::Result<void, config::ConfigError>::ok();
                }

                if (!reader.seek(start_frame)) {
                    Logger::get_instance()->warn(
                        "[VideoRunner] Seek failed, starting from beginning");
                    start_frame = 0;
                } else {
                    Logger::get_instance()->info(std::format(
                        "[VideoRunner] Resuming from frame {}/{}", start_frame, total_frames));
                }
            }
        }

        if (context.metrics_collector) {
            context.metrics_collector->set_total_frames(total_frames);
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
        if (!task_config.io.output.video_encoder.empty()) {
            video_params.videoCodec = task_config.io.output.video_encoder;
        }
        if (task_config.io.output.video_quality > 0) {
            video_params.quality = task_config.io.output.video_quality;
        }

        VideoWriter writer(video_output_path, video_params);

        // 4. Setup Pipeline
        PipelineConfig pipeline_config;
        pipeline_config.worker_thread_count = task_config.resource.get_effective_thread_count();
        pipeline_config.max_queue_size = task_config.resource.max_queue_size;

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);

        // Mutable context for processor addition (if needed)
        ProcessorContext mutable_context = context;
        auto add_result = add_processors_func(pipeline, task_config, mutable_context);
        if (!add_result) {
            timer.set_result("error:add_processors_failed");
            return add_result;
        }

        pipeline->start();

        // Initial GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        // Initial GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        std::shared_ptr<const std::vector<float>> shared_source_embedding;
        if (!context.source_embedding.empty()) {
            shared_source_embedding =
                std::make_shared<const std::vector<float>>(context.source_embedding);
        }

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        // 5. Writer Thread
        std::thread writer_thread([&]() {
            int frame_count = 0;
            int total_frames = reader.get_frame_count();
            auto start_time = std::chrono::steady_clock::now();

            TaskProgress progress;
            progress.task_id = task_config.task_info.id;
            progress.total_frames = total_frames;
            progress.current_step = "processing";

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
                    if (context.metrics_collector) context.metrics_collector->record_frame_failed();
                    break;
                }

                if (context.metrics_collector) context.metrics_collector->record_frame_completed();

                frame_count++;
                if (progress_callback) {
                    auto now = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration<double>(now - start_time).count();
                    double fps =
                        (elapsed > 0.0) ? (static_cast<double>(frame_count) / elapsed) : 0.0;

                    progress.current_frame = frame_count;
                    progress.fps = fps;
                    progress_callback(progress);
                }
            }
        });

        // 6. Reader Loop
        long long seq_id = start_frame;
        cv::Mat frame;
        int max_frames = task_config.resource.max_frames;

        int sample_counter = 0;
        const int SAMPLE_EVERY_N_FRAMES = 50;

        while (!cancelled && !writer_error) {
            if (max_frames > 0 && seq_id >= max_frames) break;

            frame = reader.read_frame();
            if (frame.empty()) break;

            FrameData data;
            data.sequence_id = seq_id++;
            data.image = frame;
            data.source_embedding = shared_source_embedding;
            pipeline->push_frame(std::move(data));

            // Periodic GPU memory sampling
            if (context.metrics_collector && ++sample_counter >= SAMPLE_EVERY_N_FRAMES) {
                sample_counter = 0;
                if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                    context.metrics_collector->record_gpu_memory(mem_info->used_mb);
                }
            }

            // Save checkpoint periodically (every 30s)
            // Optimization: Only construct and call save every 100 frames to reduce per-frame
            // overhead
            if (ckpt_mgr && seq_id % 100 == 0) {
                CheckpointData ckpt;
                ckpt.task_id = task_config.task_info.id;
                ckpt.config_hash = config_hash;
                ckpt.last_completed_frame = seq_id - 1;
                ckpt.total_frames = total_frames;
                ckpt.output_path = output_path;
                ckpt_mgr->save(ckpt);
            }
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) { writer_thread.join(); }

        // Final GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        pipeline->stop();
        writer.close();
        reader.close();

        if (cancelled) {
            std::filesystem::remove(video_output_path);
            if (needs_muxing && std::filesystem::exists(output_path))
                std::filesystem::remove(output_path);
            timer.set_result("cancelled");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E407TaskCancelled, "Task cancelled"));
        }

        if (writer_error) {
            timer.set_result("error:writer_failed");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E406OutputWriteFailed, writer_error_msg));
        }

        // Cleanup checkpoint on success
        if (ckpt_mgr) { ckpt_mgr->cleanup(task_config.task_info.id); }

        // 7. Muxing
        if (needs_muxing) {
            if (Remuxer::merge_av(video_output_path, target_path, output_path)) {
                std::filesystem::remove(video_output_path);
            } else {
                Logger::get_instance()->error(
                    config::ConfigError(config::ErrorCode::E406OutputWriteFailed,
                                        "Failed to mux audio")
                        .formatted());
                if (std::filesystem::exists(output_path)) std::filesystem::remove(output_path);
                std::filesystem::rename(video_output_path, output_path);
            }
        }

        timer.set_result("success");
        return config::Result<void, config::ConfigError>::ok();
    }

private:
    /**
     * @brief Process video in time-based segments (segment_duration_seconds > 0)
     * @details Splits the video into segments of `segment_duration_seconds` each,
     *          processes every segment through the pipeline into a temporary file,
     *          then merges all segment files into the final output (video-only,
     *          audio is muxed afterwards via Remuxer).
     */
    static config::Result<void, config::ConfigError> ProcessVideoSegmented(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<config::Result<void, config::ConfigError>(
            std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using namespace foundation::media::ffmpeg;
        using foundation::infrastructure::ScopedTimer;

        ScopedTimer timer("VideoProcessingHelper::ProcessVideoSegmented",
                          std::format("target={}", target_path));

        VideoReader reader(target_path);
        if (!reader.open()) {
            timer.set_result("error:open_failed");
            return config::Result<void, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E402VideoOpenFailed, "Failed to open video: " + target_path));
        }

        const double fps = reader.get_fps();
        const int64_t total_frames = reader.get_frame_count();
        const int64_t max_frames = task_config.resource.max_frames;
        const int64_t effective_total =
            (max_frames > 0 && max_frames < total_frames) ? max_frames : total_frames;

        // Segment boundary in frames: segment_duration_seconds * fps
        const int64_t segment_frames = std::max<int64_t>(
            1, static_cast<int64_t>(
                   std::llround(task_config.resource.segment_duration_seconds * fps)));
        const int64_t segment_count = (effective_total + segment_frames - 1) / segment_frames;

        Logger::get_instance()->info(
            std::format("[VideoRunner] Segmented processing: {} segments x {} frames (total {})",
                        segment_count, segment_frames, effective_total));

        if (context.metrics_collector) {
            context.metrics_collector->set_total_frames(effective_total);
        }

        std::string output_path = GenerateOutputPath(target_path, task_config);
        bool needs_muxing = (task_config.io.output.audio_policy == config::AudioPolicy::Copy);
        std::string final_video_path = output_path;
        if (needs_muxing) { final_video_path = output_path + ".temp.mp4"; }

        // Output params derived from source
        VideoParams video_params;
        video_params.width = reader.get_width();
        video_params.height = reader.get_height();
        video_params.frameRate = fps;
        if (!task_config.io.output.video_encoder.empty()) {
            video_params.videoCodec = task_config.io.output.video_encoder;
        }
        if (task_config.io.output.video_quality > 0) {
            video_params.quality = task_config.io.output.video_quality;
        }

        // ── Per-segment processing ──────────────────────────────────────────
        std::vector<std::string> segment_files;
        std::shared_ptr<const std::vector<float>> shared_source_embedding;
        if (!context.source_embedding.empty()) {
            shared_source_embedding =
                std::make_shared<const std::vector<float>>(context.source_embedding);
        }

        for (int64_t seg = 0; seg < segment_count; ++seg) {
            if (cancelled) break;

            const int64_t seg_start = seg * segment_frames;
            const int64_t seg_end = std::min(seg_start + segment_frames, effective_total);
            if (seg_start >= effective_total) break;

            std::string segment_path = output_path + std::format(".segment_{:03d}.temp.mp4", seg);
            segment_files.push_back(segment_path);

            if (!reader.seek(seg_start)) {
                Logger::get_instance()->error(std::format(
                    "[VideoRunner] Segment {}: seek to frame {} failed", seg, seg_start));
                timer.set_result("error:segment_seek_failed");
                return config::Result<void, config::ConfigError>::err(config::ConfigError(
                    config::ErrorCode::E402VideoOpenFailed, "Segment seek failed"));
            }

            // Pipeline per segment (isolated lifecycle; model sessions stay cached)
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

            VideoWriter writer(segment_path, video_params);
            std::atomic<bool> writer_error = false;
            std::string writer_error_msg;

            // Writer thread
            std::thread writer_thread([&]() {
                int seg_frame_count = 0;
                auto start_time = std::chrono::steady_clock::now();
                TaskProgress progress;
                progress.task_id = task_config.task_info.id;
                progress.total_frames = effective_total;
                progress.current_step = "processing";

                while (true) {
                    auto result_opt = pipeline->pop_frame();
                    if (!result_opt || result_opt->is_end_of_stream) break;

                    if (!writer.is_opened()) {
                        VideoParams actual_params = video_params;
                        actual_params.width = result_opt->image.cols;
                        actual_params.height = result_opt->image.rows;
                        writer = VideoWriter(segment_path, actual_params);
                        if (!writer.open()) {
                            writer_error = true;
                            writer_error_msg = "Failed to open segment writer";
                            break;
                        }
                    }

                    if (!writer.write_frame(result_opt->image)) {
                        writer_error = true;
                        writer_error_msg = "Failed to write segment frame";
                        break;
                    }
                    if (context.metrics_collector) {
                        context.metrics_collector->record_frame_completed();
                    }

                    seg_frame_count++;
                    if (progress_callback) {
                        auto now = std::chrono::steady_clock::now();
                        double elapsed = std::chrono::duration<double>(now - start_time).count();
                        double seg_fps = (elapsed > 0.0) ?
                                             (static_cast<double>(seg_frame_count) / elapsed) :
                                             0.0;
                        progress.current_frame = seg_start + seg_frame_count;
                        progress.fps = seg_fps;
                        progress_callback(progress);
                    }
                }
            });

            // Reader loop for this segment
            long long seq_id = seg_start;
            cv::Mat frame;
            while (!cancelled && !writer_error) {
                frame = reader.read_frame();
                if (frame.empty()) break;
                if (seq_id >= seg_end) break;

                FrameData data;
                data.sequence_id = seq_id++;
                data.image = frame;
                data.source_embedding = shared_source_embedding;
                pipeline->push_frame(std::move(data));
            }

            FrameData eos;
            eos.is_end_of_stream = true;
            eos.sequence_id = seq_id;
            pipeline->push_frame(std::move(eos));

            if (writer_thread.joinable()) { writer_thread.join(); }
            pipeline->stop();
            writer.close();

            if (writer_error) {
                timer.set_result("error:segment_writer_failed");
                return config::Result<void, config::ConfigError>::err(config::ConfigError(
                    config::ErrorCode::E406OutputWriteFailed, writer_error_msg));
            }
        }

        reader.close();

        if (cancelled) {
            for (const auto& sf : segment_files) {
                if (std::filesystem::exists(sf)) std::filesystem::remove(sf);
            }
            timer.set_result("cancelled");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E407TaskCancelled, "Task cancelled"));
        }

        // ── Merge segment files into final video output ────────────────────
        VideoWriter final_writer(final_video_path, video_params);
        for (const auto& sf : segment_files) {
            if (!std::filesystem::exists(sf)) {
                Logger::get_instance()->error(
                    std::format("[VideoRunner] Missing segment file: {}", sf));
                continue;
            }
            VideoReader seg_reader(sf);
            if (!seg_reader.open()) {
                Logger::get_instance()->error(
                    std::format("[VideoRunner] Failed to open segment: {}", sf));
                timer.set_result("error:segment_merge_failed");
                return config::Result<void, config::ConfigError>::err(config::ConfigError(
                    config::ErrorCode::E406OutputWriteFailed, "Segment merge failed"));
            }
            while (true) {
                cv::Mat seg_frame = seg_reader.read_frame();
                if (seg_frame.empty()) break;
                if (!final_writer.is_opened()) {
                    VideoParams actual_params = video_params;
                    actual_params.width = seg_frame.cols;
                    actual_params.height = seg_frame.rows;
                    final_writer = VideoWriter(final_video_path, actual_params);
                    if (!final_writer.open()) {
                        timer.set_result("error:final_writer_failed");
                        return config::Result<void, config::ConfigError>::err(
                            config::ConfigError(config::ErrorCode::E406OutputWriteFailed,
                                                "Failed to open final writer"));
                    }
                }
                if (!final_writer.write_frame(seg_frame)) {
                    timer.set_result("error:final_writer_failed");
                    return config::Result<void, config::ConfigError>::err(config::ConfigError(
                        config::ErrorCode::E406OutputWriteFailed, "Failed to write merged frame"));
                }
            }
            seg_reader.close();
        }
        final_writer.close();

        // Cleanup segment files
        for (const auto& sf : segment_files) {
            if (std::filesystem::exists(sf)) std::filesystem::remove(sf);
        }

        // Final GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        // ── Mux audio (same as normal path) ─────────────────────────────────
        if (needs_muxing) {
            if (Remuxer::merge_av(final_video_path, target_path, output_path)) {
                std::filesystem::remove(final_video_path);
            } else {
                Logger::get_instance()->error(
                    config::ConfigError(config::ErrorCode::E406OutputWriteFailed,
                                        "Failed to mux audio")
                        .formatted());
                if (std::filesystem::exists(output_path)) std::filesystem::remove(output_path);
                std::filesystem::rename(final_video_path, output_path);
            }
        }

        timer.set_result("success");
        return config::Result<void, config::ConfigError>::ok();
    }

    /**
     * @brief Process video in strict mode (optimized for low memory usage)
     */
    static config::Result<void, config::ConfigError> ProcessVideoStrict(
        const std::string& target_path, const config::TaskConfig& task_config,
        ProgressCallback progress_callback, const ProcessorContext& context,
        std::function<config::Result<void, config::ConfigError>(
            std::shared_ptr<Pipeline>, const config::TaskConfig&, ProcessorContext&)>
            add_processors_func,
        std::atomic<bool>& cancelled) {
        using namespace foundation::media::ffmpeg;
        using foundation::infrastructure::ScopedTimer;

        ScopedTimer timer("VideoProcessingHelper::ProcessVideoStrict",
                          std::format("target={}", target_path));

        VideoReader reader(target_path);
        if (!reader.open()) {
            timer.set_result("error:open_failed");
            return config::Result<void, config::ConfigError>::err(config::ConfigError(
                config::ErrorCode::E402VideoOpenFailed, "Failed to open video: " + target_path));
        }

        int64_t start_frame = 0;
        int64_t total_frames = reader.get_frame_count();
        std::unique_ptr<CheckpointManager> ckpt_mgr;
        std::string config_hash;

        // Try to resume if enabled
        if (task_config.task_info.enable_resume) {
            ckpt_mgr = std::make_unique<CheckpointManager>("./checkpoints");
            config_hash = CalculateConfigHash(task_config);

            if (auto saved = ckpt_mgr->load(task_config.task_info.id, config_hash)) {
                start_frame = saved->last_completed_frame + 1;

                if (start_frame >= total_frames) {
                    Logger::get_instance()->info(
                        "[VideoRunnerStrict] Task already completed, nothing to resume");
                    ckpt_mgr->cleanup(task_config.task_info.id);
                    return config::Result<void, config::ConfigError>::ok();
                }

                if (!reader.seek(start_frame)) {
                    Logger::get_instance()->warn(
                        "[VideoRunnerStrict] Seek failed, starting from beginning");
                    start_frame = 0;
                } else {
                    Logger::get_instance()->info(
                        std::format("[VideoRunnerStrict] Resuming from frame {}/{}", start_frame,
                                    total_frames));
                }
            }
        }

        if (context.metrics_collector) {
            context.metrics_collector->set_total_frames(total_frames);
        }

        std::string output_path = GenerateOutputPath(target_path, task_config);
        std::string video_output_path = output_path;
        bool needs_muxing = (task_config.io.output.audio_policy == config::AudioPolicy::Copy);
        if (needs_muxing) { video_output_path = output_path + ".temp.mp4"; }

        VideoParams video_params;
        video_params.width = reader.get_width();
        video_params.height = reader.get_height();
        video_params.frameRate = reader.get_fps();
        if (!task_config.io.output.video_encoder.empty()) {
            video_params.videoCodec = task_config.io.output.video_encoder;
        }
        if (task_config.io.output.video_quality > 0) {
            video_params.quality = task_config.io.output.video_quality;
        }

        VideoWriter writer(video_output_path, video_params);

        PipelineConfig pipeline_config;
        // In strict memory mode, we still respect config but might want to cap it if it's too
        // large. For now, let's just use the config value as requested.
        pipeline_config.max_queue_size = std::min(task_config.resource.max_queue_size, 4);
        pipeline_config.worker_thread_count = task_config.resource.get_effective_thread_count();

        auto pipeline = std::make_shared<Pipeline>(pipeline_config);
        ProcessorContext mutable_context = context;
        auto add_result = add_processors_func(pipeline, task_config, mutable_context);
        if (!add_result) {
            timer.set_result("error:add_processors_failed");
            return add_result;
        }
        pipeline->start();

        // Initial GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        std::shared_ptr<const std::vector<float>> shared_source_embedding;
        if (!context.source_embedding.empty()) {
            shared_source_embedding =
                std::make_shared<const std::vector<float>>(context.source_embedding);
        }

        std::atomic<bool> writer_error = false;
        std::string writer_error_msg;

        std::thread writer_thread([&]() {
            int frame_count = 0;
            int total_frames = reader.get_frame_count();
            auto start_time = std::chrono::steady_clock::now();

            TaskProgress progress;
            progress.task_id = task_config.task_info.id;
            progress.total_frames = total_frames;
            progress.current_step = "processing";

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
                    if (context.metrics_collector) context.metrics_collector->record_frame_failed();
                    break;
                }
                if (context.metrics_collector) context.metrics_collector->record_frame_completed();
                frame_count++;
                if (progress_callback) {
                    auto now = std::chrono::steady_clock::now();
                    double elapsed = std::chrono::duration<double>(now - start_time).count();
                    double fps =
                        (elapsed > 0.0) ? (static_cast<double>(frame_count) / elapsed) : 0.0;

                    progress.current_frame = frame_count;
                    progress.fps = fps;
                    progress_callback(progress);
                }
            }
        });

        long long seq_id = start_frame;
        cv::Mat frame;

        int sample_counter = 0;
        const int SAMPLE_EVERY_N_FRAMES = 50;

        while (!cancelled && !writer_error) {
            frame = reader.read_frame();
            if (frame.empty()) break;
            FrameData data;
            data.sequence_id = seq_id++;
            data.image = frame;
            data.source_embedding = shared_source_embedding;
            pipeline->push_frame(std::move(data));

            // Periodic GPU memory sampling
            if (context.metrics_collector && ++sample_counter >= SAMPLE_EVERY_N_FRAMES) {
                sample_counter = 0;
                if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                    context.metrics_collector->record_gpu_memory(mem_info->used_mb);
                }
            }

            // Save checkpoint periodically (every 30s)
            if (ckpt_mgr) {
                CheckpointData ckpt;
                ckpt.task_id = task_config.task_info.id;
                ckpt.config_hash = config_hash;
                ckpt.last_completed_frame = seq_id - 1;
                ckpt.total_frames = total_frames;
                ckpt.output_path = output_path;
                ckpt_mgr->save(ckpt);
            }
        }

        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = seq_id;
        pipeline->push_frame(std::move(eos));

        if (writer_thread.joinable()) writer_thread.join();

        // Final GPU memory sample
        if (context.metrics_collector) {
            if (auto mem_info = foundation::infrastructure::cuda::get_gpu_memory_info()) {
                context.metrics_collector->record_gpu_memory(mem_info->used_mb);
            }
        }

        pipeline->stop();
        writer.close();
        reader.close();

        if (cancelled) {
            std::filesystem::remove(video_output_path);
            if (needs_muxing && std::filesystem::exists(output_path))
                std::filesystem::remove(output_path);
            timer.set_result("cancelled");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E407TaskCancelled, "Task cancelled"));
        }
        if (writer_error) {
            timer.set_result("error:writer_failed");
            return config::Result<void, config::ConfigError>::err(
                config::ConfigError(config::ErrorCode::E406OutputWriteFailed, writer_error_msg));
        }

        // Cleanup checkpoint on success
        if (ckpt_mgr) { ckpt_mgr->cleanup(task_config.task_info.id); }

        if (needs_muxing) {
            if (Remuxer::merge_av(video_output_path, target_path, output_path)) {
                std::filesystem::remove(video_output_path);
            } else {
                Logger::get_instance()->error(
                    config::ConfigError(config::ErrorCode::E406OutputWriteFailed,
                                        "Failed to mux audio")
                        .formatted());
                if (std::filesystem::exists(output_path)) std::filesystem::remove(output_path);
                std::filesystem::rename(video_output_path, output_path);
            }
        }
        timer.set_result("success");
        return config::Result<void, config::ConfigError>::ok();
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

    /**
     * @brief Calculate SHA1 hash of task configuration for consistency check
     */
    static std::string CalculateConfigHash(const config::TaskConfig& task_config) {
        // Basic serialization of key task parameters to detect changes
        std::stringstream ss;
        ss << task_config.task_info.id;
        for (const auto& path : task_config.io.target_paths) ss << path;
        for (const auto& path : task_config.io.source_paths) ss << path;
        ss << task_config.io.output.path;
        ss << task_config.io.output.video_encoder;
        ss << task_config.io.output.video_quality;
        for (const auto& step : task_config.pipeline) {
            if (step.enabled) ss << step.step;
        }
        return foundation::infrastructure::crypto::sha1_string(ss.str());
    }
};

} // namespace services::pipeline
