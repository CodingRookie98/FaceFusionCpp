/**
 ******************************************************************************
 * @file           : core.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <filesystem>
#include <future>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <chrono>

module core;
import core_options;
import :core_task;
import face_store;
import face_selector;
import vision;
import file_system;
import crypto;
import ffmpeg_runner;
import processor_hub;
import face_swapper;
import progress_bar;
import metadata;
import thread_pool;
// import job_manager;
import utils;

using namespace std;
using namespace ffc;
using namespace faceSwapper;
using namespace faceEnhancer;
using namespace expressionRestore;
using namespace frame_enhancer;

namespace ffc::core {

using namespace task;
using namespace media;
using namespace infra;

Core::Core(const CoreOptions& options) :
    m_processor_hub(options.inference_session_options) {
    m_core_options = options;
    Logger::get_instance()->set_log_level(m_core_options.logger_options.log_level);
    m_logger = Logger::get_instance();
    m_thread_pool.reset(m_core_options.task_options.per_task_thread_count);

    if (m_core_options.model_options.force_download) {
        // Todo: Download all model if force_download is true
        // if (!ModelManager::get_instance()->downloadAllModel()) {
        //     logger_->error("[Core] Download all model failed.");
        //     return;
        // }
    }
}

Core::~Core() {
    file_system::remove_dir(file_system::get_temp_path() + "/" + metadata::name);
}

bool Core::run_task(Task task) {
    for (const auto& [type, model, param] : task.processors_info) {
        if (type == ProcessorMajorType::FaceSwapper
            || type == ProcessorMajorType::FaceEnhancer
            || type == ProcessorMajorType::ExpressionRestorer) {
            if (m_face_analyser == nullptr) {
                m_face_analyser = std::make_shared<FaceAnalyser>(m_env, m_core_options.inference_session_options);
            }
        }
    }

    return true;
}

bool Core::ProcessVideo(CoreTask core_task) {
    if (core_task.target_paths.size() > 1) {
        m_logger->warn(std::format("{}: Only one target video is supported. Only the first video is processed this time. \n{}", __FUNCTION__, core_task.target_paths.front()));
    }

    std::string& videoPath = core_task.target_paths.front();

    std::string audiosDir = file_system::absolute_path(file_system::parent_path(videoPath) + "/audios");
    if (!core_task.skip_audio) {
        Audio_Codec audioCodec = getAudioCodec(core_task.output_audio_encoder.value());
        if (audioCodec == Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        extractAudios(videoPath, audiosDir, audioCodec);
    }

    m_logger->info(std::format("[Core] Extract Frames for {}", videoPath));
    std::string pattern = "frame_%06d." + core_task.temp_frame_format.value();
    std::string videoFramesOutputDir = file_system::absolute_path(file_system::parent_path(videoPath) + "/" + file_system::get_base_name(videoPath));
    std::string outputPattern = videoFramesOutputDir + "/" + pattern;
    extractFrames(videoPath, outputPattern);
    std::unordered_set<std::string> framePaths = file_system::list_files(videoFramesOutputDir);
    framePaths = vision::filter_image_paths(framePaths);
    std::vector<std::string> framePathsVec(framePaths.begin(), framePaths.end());

    CoreTask tmpCoreRunOptions = core_task;
    tmpCoreRunOptions.target_paths = framePathsVec;
    tmpCoreRunOptions.output_paths = framePathsVec;
    ProcessImages(tmpCoreRunOptions);

    VideoPrams videoPrams(videoPath);
    videoPrams.quality = core_task.output_video_quality.value();
    videoPrams.preset = core_task.output_video_preset.value();
    videoPrams.videoCodec = core_task.output_video_encoder.value();
    if (!framePathsVec.empty()) {
        cv::Mat firstFrame = vision::read_static_image(framePathsVec[0]);
        videoPrams.width = firstFrame.cols;
        videoPrams.height = firstFrame.rows;
    }

    std::string inputImagePattern = videoFramesOutputDir + "/" + pattern;
    std::string outputVideo_NA_Path = file_system::parent_path(videoPath) + "/" + file_system::get_base_name(videoPath) + "_processed_NA" + file_system::get_file_ext(videoPath);
    Logger::get_instance()->info("[Core] Images to video : " + file_system::absolute_path(outputVideo_NA_Path));
    if (!imagesToVideo(inputImagePattern, outputVideo_NA_Path, videoPrams)) {
        Logger::get_instance()->error("[Core] images to video failed!");
        file_system::remove_dir(videoFramesOutputDir);
        file_system::remove_file(outputVideo_NA_Path);
        return false;
    }

    if (!core_task.skip_audio) {
        std::unordered_set<std::string> audioPaths = file_system::list_files(audiosDir);
        audioPaths = filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::get_instance()->info("[Core] Add audios to video : " + file_system::absolute_path(core_task.output_paths.front()));
        if (!addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, core_task.output_paths.front())) {
            Logger::get_instance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            file_system::move_file(outputVideo_NA_Path, core_task.output_paths.front());
        } catch (const std::exception& e) {
            Logger::get_instance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    file_system::remove_dir(videoFramesOutputDir);
    file_system::remove_dir(audiosDir);
    file_system::remove_file(outputVideo_NA_Path);
    return true;
}

bool Core::ProcessVideoInSegments(CoreTask core_task) {
    std::string& videoPath = core_task.target_paths.front();

    std::string audiosDir = file_system::absolute_path(file_system::parent_path(videoPath) + "/audios");
    if (!core_task.skip_audio) {
        Audio_Codec audioCodec = getAudioCodec(core_task.output_audio_encoder.value());
        if (audioCodec == Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        extractAudios(videoPath, audiosDir, audioCodec);
    }

    std::string videoSegmentsDir = file_system::parent_path(videoPath) + "/videoSegments";
    std::string videoSegmentPattern = "segment_%03d" + file_system::get_file_ext(videoPath);
    Logger::get_instance()->info(std::format("[Core] Divide the video into segments of {} seconds each....", core_task.video_segment_duration.value()));
    if (!cutVideoIntoSegments(videoPath, videoSegmentsDir,
                              core_task.video_segment_duration.value(), videoSegmentPattern)) {
        Logger::get_instance()->error("The attempt to cut the video into segments was failed!");
        file_system::remove_dir(audiosDir);
        return false;
    }

    std::unordered_set<std::string> videoSegmentsPathsSet = file_system::list_files(videoSegmentsDir);
    videoSegmentsPathsSet = filterVideoPaths(videoSegmentsPathsSet);

    std::vector<std::string> processedVideoSegmentsPaths;
    std::string processedVideoSegmentsDir = file_system::parent_path(videoPath) + "/videoSegments_processed";
    std::vector<std::string> videoSegmentsPathsVec(videoSegmentsPathsSet.begin(), videoSegmentsPathsSet.end());
    std::ranges::sort(videoSegmentsPathsVec, [](const std::string& a, const std::string& b) {
        return a < b;
    });

    for (size_t segmentIndex = 0; segmentIndex < videoSegmentsPathsVec.size(); ++segmentIndex) {
        const auto& videoSegmentPath = videoSegmentsPathsVec.at(segmentIndex);
        std::string outputVideoSegmentPath = file_system::absolute_path(processedVideoSegmentsDir + "/" + file_system::get_file_name(videoSegmentPath));

        CoreTask tmpCoreRunOptions = core_task;
        tmpCoreRunOptions.target_paths = {videoSegmentPath};
        tmpCoreRunOptions.output_paths = {outputVideoSegmentPath};
        tmpCoreRunOptions.skip_audio = true;

        m_logger->info(std::format("[Core] Processing video segment {}/{}", segmentIndex + 1, videoSegmentsPathsSet.size()));
        if (!ProcessVideo(tmpCoreRunOptions)) {
            m_logger->error(std::format("[Core] Failed to process video segment: {}", videoSegmentPath));
            return false;
        }
        processedVideoSegmentsPaths.emplace_back(outputVideoSegmentPath);

        file_system::remove_file(videoSegmentPath);
    }
    file_system::remove_dir(videoSegmentsDir);

    VideoPrams videoPrams(processedVideoSegmentsPaths[0]);
    videoPrams.quality = core_task.output_image_quality.value();
    videoPrams.preset = core_task.output_video_preset.value();
    videoPrams.videoCodec = core_task.output_video_encoder.value();

    std::string outputVideo_NA_Path = file_system::parent_path(videoPath) + "/" + file_system::get_base_name(videoPath) + "_processed_NA" + file_system::get_file_ext(videoPath);
    Logger::get_instance()->info("[Core] concat video segments...");
    if (!concatVideoSegments(processedVideoSegmentsPaths, outputVideo_NA_Path, videoPrams)) {
        Logger::get_instance()->error("[Core] Failed concat video segments for : " + videoPath);
        file_system::remove_dir(processedVideoSegmentsDir);
        return false;
    }

    if (!core_task.skip_audio) {
        std::unordered_set<std::string> audioPaths = file_system::list_files(audiosDir);
        audioPaths = filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::get_instance()->info("[Core] Add audios to video...");
        if (!addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, core_task.output_paths.front())) {
            Logger::get_instance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            file_system::move_file(outputVideo_NA_Path, core_task.output_paths.front());
        } catch (const std::exception& e) {
            Logger::get_instance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    file_system::remove_dir(audiosDir);
    file_system::remove_file(outputVideo_NA_Path);
    file_system::remove_dir(processedVideoSegmentsDir);
    return true;
}

bool Core::ProcessVideos(const CoreTask& core_task, const bool& autoRemoveTarget) {
    if (core_task.target_paths.empty()) {
        m_logger->error(" videoPaths is empty.");
        return false;
    }
    if (core_task.output_paths.empty()) {
        m_logger->error("[Core::ProcessVideos] outputVideoPaths is empty.");
        return false;
    }
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        m_logger->error("[Core::ProcessVideos] videoPaths and outputVideoPaths size mismatch.");
        return false;
    }

    bool isAllSuccess = true;
    for (size_t i = 0; i < core_task.target_paths.size(); ++i) {
        CoreTask tmpCoreRunOptions = core_task;
        tmpCoreRunOptions.target_paths = {core_task.target_paths[i]};
        tmpCoreRunOptions.output_paths = {core_task.output_paths[i]};
        m_logger->info(std::format("Processing video {}/{}", i + 1, core_task.target_paths.size()));
        if (core_task.video_segment_duration.value() > 0) {
            if (ProcessVideoInSegments(tmpCoreRunOptions)) {
                m_logger->info(std::format("[Core] Video processed successfully. Output path: {} ", core_task.output_paths[i]));
            } else {
                isAllSuccess = false;
                m_logger->error(std::format("[Core] Video {} processed failed.", core_task.target_paths[i]));
            }
        } else {
            if (ProcessVideo(tmpCoreRunOptions)) {
                m_logger->info(std::format("[Core] Video processed successfully. Output path: {} ", core_task.output_paths[i]));
            } else {
                isAllSuccess = false;
                m_logger->error(std::format("[Core] Video {} processed failed.", core_task.target_paths[i]));
            }
        }
        if (autoRemoveTarget) {
            file_system::remove_file(core_task.target_paths[i]);
        }
    }
    return isAllSuccess;
}

bool Core::Run(CoreTask core_task) {
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        m_logger->error("[Core::Run] target_paths and output_paths size mismatch.");
        return false;
    }

    if (core_task.processor_model.contains(ProcessorMajorType::FaceSwapper)
        || core_task.processor_model.contains(ProcessorMajorType::FaceEnhancer)
        || core_task.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (m_face_analyser == nullptr) {
            m_face_analyser = std::make_shared<FaceAnalyser>(m_env, m_core_options.inference_session_options);
        }
    }

    std::string tmpPath;
    do {
        std::string id = utils::generate_random_str(10);
        tmpPath = file_system::get_temp_path() + "/" + metadata::name + "/" + id;
        core_task.source_average_face_id = id;
    } while (file_system::dir_exists(tmpPath));

    if (core_task.processor_minor_types.contains(ProcessorMajorType::FaceSwapper)
        && core_task.processor_minor_types.at(ProcessorMajorType::FaceSwapper) == ProcessorMinorType::FaceSwapper_InSwapper) {
        std::unordered_set<std::string> sourceImgPaths(core_task.source_paths->begin(), core_task.source_paths->end());
        core_task.source_average_face_id = {crypto::combined_sha1(sourceImgPaths)};
        core_task.source_average_face = std::make_shared<Face>(core_task.ProcessSourceAverageFace(m_face_analyser));
    }

    std::vector<string> tmpTargetImgPaths, outputImgPaths;
    std::vector<string> tmpTargetVideoPaths, outputVideoPaths;
    for (size_t i = 0; i < core_task.target_paths.size(); ++i) {
        const std::string& targetPath = core_task.target_paths[i];
        const std::string& outputPath = core_task.output_paths[i];
        if (vision::is_image(targetPath)) {
            std::string str = tmpPath + "/images/" + file_system::get_file_name(targetPath);
            file_system::copy(targetPath, str);
            tmpTargetImgPaths.emplace_back(str);
            outputImgPaths.emplace_back(outputPath);
        } else if (vision::is_video(targetPath)) {
            const std::string file_symlink_path = tmpPath + "/videos/" + file_system::get_file_name(targetPath);
            file_system::create_dir(tmpPath + "/videos");
            try {
                std::filesystem::create_symlink(targetPath, file_symlink_path);
            } catch (std::filesystem::filesystem_error& e) {
                m_logger->error(std::format("[Core::Run] Create symlink failed! Error: {}", e.what()));
            } catch (std::exception& e) {
                m_logger->error(std::format("[Core::Run] Create symlink failed! Error: {}", e.what()));
            }
            tmpTargetVideoPaths.emplace_back(file_symlink_path);
            outputVideoPaths.emplace_back(outputPath);
        }
    }

    // process images;
    bool imagesAllOK = true;
    if (!tmpTargetImgPaths.empty() && tmpTargetImgPaths.size() == outputImgPaths.size()) {
        CoreTask tmpRunOptions = core_task;
        tmpRunOptions.target_paths = tmpTargetImgPaths;
        tmpRunOptions.output_paths = tmpTargetImgPaths;
        m_logger->info("[Core] Processing Images...");
        if (!ProcessImages(tmpRunOptions)) {
            imagesAllOK = false;
        }
        file_system::move_files(tmpTargetImgPaths, outputImgPaths);
    }

    // process videos;
    bool videosAllOK = true;
    if (!tmpTargetVideoPaths.empty() && tmpTargetVideoPaths.size() == outputVideoPaths.size()) {
        CoreTask tmpRunOptions = core_task;
        tmpRunOptions.target_paths = tmpTargetVideoPaths;
        tmpRunOptions.output_paths = outputVideoPaths;
        m_logger->info("[Core] Processing Videos...");
        if (!ProcessVideos(tmpRunOptions, true)) {
            videosAllOK = false;
        }
    }

    file_system::remove_dir(tmpPath);
    return imagesAllOK && videosAllOK;
}

bool Core::ProcessImages(CoreTask core_task) {
    if (core_task.target_paths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (core_task.output_paths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        m_logger->error(std::string(__FUNCTION__) + " target_paths and output_paths size mismatch");
        return false;
    }

    if (core_task.processor_model.contains(ProcessorMajorType::FaceSwapper)
        && core_task.processor_minor_types[ProcessorMajorType::FaceSwapper] == ProcessorMinorType::FaceSwapper_InSwapper) {
        if (core_task.source_average_face == nullptr) {
            core_task.source_average_face = std::make_shared<Face>(core_task.ProcessSourceAverageFace(m_face_analyser));
        }
    }
    if (core_task.processor_list.front() == ProcessorMajorType::ExpressionRestorer) {
        if (!core_task.source_paths.has_value()) {
            m_logger->error(std::string(__FUNCTION__) + " source_paths is nullopt");
        } else if (core_task.source_paths->empty()) {
            m_logger->error(std::string(__FUNCTION__) + " source_paths is empty");
        } else if (core_task.target_paths.size() != core_task.source_paths->size()) {
            m_logger->error(std::string(__FUNCTION__) + " target_paths and source_paths size mismatch");
        }
    }

    std::vector<std::string> originalTargetPaths;
    if (core_task.processor_minor_types.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (core_task.processor_list.front() != ProcessorMajorType::ExpressionRestorer) {
            for (const auto& path : core_task.target_paths) {
                const std::string tmpSourcePath = file_system::parent_path(path) + "/" + file_system::get_base_name(path) + "_original" + file_system::get_file_ext(path);
                originalTargetPaths.emplace_back(tmpSourcePath);
            }
            file_system::copy_files(core_task.target_paths, originalTargetPaths);
        } else {
            if (!core_task.source_paths.has_value()) {
                core_task.source_paths = {{}};
            }
            originalTargetPaths = core_task.source_paths.value();
        }
    }

    for (auto itr = core_task.target_paths.begin(); itr != core_task.target_paths.end();) {
        if (!vision::is_image(*itr)) {
            m_logger->warn(std::string(__FUNCTION__) + " target_path is not image: " + *itr);
            itr = core_task.target_paths.erase(itr);
            continue;
        }
        ++itr;
    }

    std::unique_ptr<CoreTask> core_task_for_ER = nullptr;
    for (const auto& type : core_task.processor_list) {
        if (type == ProcessorMajorType::ExpressionRestorer) {
            if (originalTargetPaths.size() != core_task.target_paths.size()) {
                m_logger->error(std::string(__FUNCTION__) + std::format(" source Paths size is {}, but target paths size is {}!", originalTargetPaths.size(), core_task.target_paths.size()));
                return false;
            }
            if (core_task_for_ER == nullptr) {
                core_task_for_ER = std::make_unique<CoreTask>(core_task);
                core_task_for_ER->source_paths = originalTargetPaths;
            }
        }

        auto func_to_process = [&](const size_t index) -> bool {
            if (type == ProcessorMajorType::FaceSwapper) {
                return SwapFace(core_task.GetFaceSwapperInput(index, m_face_analyser),
                                core_task.output_paths.at(index),
                                get_face_swapper_type(core_task.processor_minor_types[type]).value(),
                                core_task.processor_model[type]);
            }
            if (type == ProcessorMajorType::FaceEnhancer) {
                return EnhanceFace(core_task.GetFaceEnhancerInput(index, m_face_analyser),
                                   core_task.output_paths.at(index),
                                   get_face_enhancer_type(core_task.processor_minor_types[type]).value(),
                                   core_task.processor_model[type]);
            }
            if (type == ProcessorMajorType::ExpressionRestorer) {
                if (originalTargetPaths.empty()) {
                    m_logger->error("ExpressionRestorer need source!");
                    return false;
                }
                CoreTask tmpCoreRunOptions = core_task;
                tmpCoreRunOptions.source_paths = originalTargetPaths;
                return RestoreExpression(tmpCoreRunOptions.GetExpressionRestorerInput(index, index, m_face_analyser),
                                         core_task.output_paths.at(index),
                                         get_expression_restorer_type(core_task.processor_minor_types[type]).value());
            }
            if (type == ProcessorMajorType::FrameEnhancer) {
                return EnhanceFrame(core_task.GetFrameEnhancerInput(index),
                                    core_task.output_paths.at(index),
                                    get_frame_enhancer_type(core_task.processor_minor_types[type]).value(),
                                    core_task.processor_model[type]);
            }
            return false;
        };

        std::unique_ptr<ProgressBar> progress_bar{nullptr};
        std::string processor_name;
        if (core_task.show_progress_bar) {
            progress_bar = std::make_unique<ProgressBar>();
            if (type == ProcessorMajorType::FaceSwapper) {
                processor_name = m_processor_hub.getProcessorPool().get_face_swapper(
                                                                       get_face_swapper_type(core_task.processor_minor_types[type]).value(),
                                                                       core_task.processor_model[type])
                                     ->get_processor_name();
            }
            if (type == ProcessorMajorType::FaceEnhancer) {
                processor_name = m_processor_hub.getProcessorPool().get_face_enhancer(
                                                                       get_face_enhancer_type(core_task.processor_minor_types[type]).value(),
                                                                       core_task.processor_model[type])
                                     ->get_processor_name();
            }
            if (type == ProcessorMajorType::ExpressionRestorer) {
                processor_name = m_processor_hub.getProcessorPool().get_expression_restorer(
                                                                       get_expression_restorer_type(core_task.processor_minor_types[type]).value())
                                     ->get_processor_name();
            }
            if (type == ProcessorMajorType::FrameEnhancer) {
                processor_name = m_processor_hub.getProcessorPool().get_frame_enhancer(
                                                                       get_frame_enhancer_type(core_task.processor_minor_types[type]).value(),
                                                                       core_task.processor_model[type])
                                     ->get_processor_name();
            }

            const size_t numTargetPaths = core_task.target_paths.size();
            ProgressBar::show_console_cursor(false);
            progress_bar->set_max_progress(100);
            progress_bar->set_prefix_text(std::format("[{}] Processing ", processor_name));
            progress_bar->set_postfix_text(std::format("{}/{}", 0, numTargetPaths));
            progress_bar->set_progress(0);
        }

        if (m_observer) {
            m_observer->on_start(core_task.target_paths.size());
        }

        std::vector<std::future<bool>> futures;
        futures.reserve(core_task.target_paths.size());

        for (size_t index = 0; index < core_task.target_paths.size(); ++index) {
            futures.push_back(m_thread_pool.enqueue(func_to_process, index));
        }

        bool is_all_success = true;
        for (size_t k = 0; k < futures.size(); ++k) {
            // Wait for result with pause support
            bool result_obtained = false;
            bool is_success = false;

            while (!result_obtained) {
                // Use wait_for with short timeout to allow checking pause state
                const std::future_status status = futures[k].wait_for(std::chrono::milliseconds(100));

                if (status == std::future_status::ready) {
                    is_success = futures[k].get();
                    result_obtained = true;
                } else {
                    // Timeout - can continue to check pause state or just retry
                    // The actual pause handling is done in JobManager via observer callbacks
                    continue;
                }
            }

            if (!is_success) {
                is_all_success = false;
                m_logger->error(std::format("[{}] Failed to write image: {}", processor_name, core_task.output_paths[k]));
            }

            if (core_task.show_progress_bar) {
                progress_bar->set_postfix_text(std::format("{}/{}", k + 1, futures.size()));
                const int progress = static_cast<int>(std::floor(static_cast<float>(k + 1) * 100.0f) / static_cast<float>(futures.size()));
                progress_bar->set_progress(progress);
            }

            if (m_observer) {
                m_observer->on_progress(k + 1, std::format("Processing {}/{}", k + 1, futures.size()));
            }
        }

        if (core_task.show_progress_bar) {
            ProgressBar::show_console_cursor(true);
        }
        if (!is_all_success) {
            m_logger->error(std::format("[{}] Some images failed to process or write.", processor_name));
        }

        if (m_observer) {
            if (is_all_success) m_observer->on_complete();
            else m_observer->on_error("Some images failed to process");
        }

        if (m_core_options.memory_options.processor_memory_strategy == CoreOptions::MemoryStrategy::Strict) {
            m_processor_hub.getProcessorPool().remove_processors(type);
        }
    }

    if (core_task.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        file_system::remove_files(originalTargetPaths);
    }
    return true;
}

bool Core::SwapFace(const FaceSwapperInput& face_swapper_input, const std::string& output_path,
                    const FaceSwapperType& type, const model_manager::Model& model) {
    if (output_path.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat swapped_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FaceSwapperType::InSwapper) {
        target_frame = face_swapper_input.in_swapper_input->target_frame;
        swapped_frame = m_processor_hub.swapFace(FaceSwapperType::InSwapper, model, face_swapper_input);
    }

    if (!swapped_frame.empty()) {
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, swapped_frame, output_path);
    } else {
        m_logger->error(std::format("Swap face failed! Result frame is empty. And target_frame is {}!",
                                    target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, *target_frame, output_path);
    }

    return true;
}

bool Core::EnhanceFace(const FaceEnhancerInput& face_enhancer_input,
                       const std::string& output_path, const FaceEnhancerType& type,
                       const model_manager::Model& model) {
    if (output_path.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat enhanced_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FaceEnhancerType::CodeFormer) {
        target_frame = face_enhancer_input.code_former_input->target_frame;
        enhanced_frame = m_processor_hub.enhanceFace(FaceEnhancerType::CodeFormer, model, face_enhancer_input);
    }
    if (type == FaceEnhancerType::GFP_GAN) {
        target_frame = face_enhancer_input.gfp_gan_input->target_frame;
        enhanced_frame = m_processor_hub.enhanceFace(FaceEnhancerType::GFP_GAN, model, face_enhancer_input);
    }

    if (!enhanced_frame.empty()) {
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, enhanced_frame, output_path);
    } else {
        m_logger->error(std::format("Enhance face failed! Result frame is empty. And target_frame is {}!",
                                    target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, *target_frame, output_path);
    }

    return true;
}

bool Core::RestoreExpression(const ExpressionRestorerInput& expression_restorer_input,
                             const std::string& output_path,
                             const ExpressionRestorerType& type) {
    if (output_path.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat restored_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == ExpressionRestorerType::LivePortrait) {
        target_frame = expression_restorer_input.live_portrait_input->target_frame;
        restored_frame = m_processor_hub.restoreExpression(ExpressionRestorerType::LivePortrait,
                                                           expression_restorer_input);
    }

    if (!restored_frame.empty()) {
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, restored_frame, output_path);
    } else {
        m_logger->error(std::format("Restore expression failed! Result frame is empty. And target_frame is {}!",
                                    target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, *target_frame, output_path);
    }

    return true;
}

bool Core::EnhanceFrame(const FrameEnhancerInput& frame_enhancer_input,
                        const std::string& output_path, const FrameEnhancerType& type,
                        const model_manager::Model& model) {
    if (output_path.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat enhanced_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FrameEnhancerType::Real_esr_gan) {
        target_frame = frame_enhancer_input.real_esr_gan_input->target_frame;
        enhanced_frame = m_processor_hub.enhanceFrame(FrameEnhancerType::Real_esr_gan, model, frame_enhancer_input);
    }
    if (type == FrameEnhancerType::Real_hat_gan) {
        target_frame = frame_enhancer_input.real_hat_gan_input->target_frame;
        enhanced_frame = m_processor_hub.enhanceFrame(FrameEnhancerType::Real_hat_gan, model, frame_enhancer_input);
    }

    if (!enhanced_frame.empty()) {
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, enhanced_frame, output_path);
    } else {
        m_logger->error(std::format("Enhance frame failed! Result frame is empty. And target_frame is {}!",
                                    target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::instance()->enqueue(vision::write_image, *target_frame, output_path);
    }

    return true;
}

} // namespace ffc::core