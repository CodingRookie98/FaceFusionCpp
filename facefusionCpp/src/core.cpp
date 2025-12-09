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

module core;
import core_options;
import :core_task;
import face_store;
import face_selector;
import vision;
import file_system;
import ffmpeg_runner;
import processor_hub;
import face_swapper;
import progress_bar;
import metadata;
import thread_pool;

using namespace std;
using namespace ffc;
using namespace faceSwapper;
using namespace faceEnhancer;
using namespace expressionRestore;
using namespace frameEnhancer;

namespace ffc {
Core::Core(const CoreOptions& options) :
    processor_hub_(options.inference_session_options) {
    core_options_ = options;
    Logger::getInstance()->setLogLevel(core_options_.log_level);
    logger_ = Logger::getInstance();

    if (core_options_.force_download) {
        if (!ModelManager::getInstance()->downloadAllModel()) {
            logger_->error("[Core] Download all model failed.");
            return;
        }
    }
}

Core::~Core() {
    FileSystem::removeDir(FileSystem::getTempPath() + "/" + metadata::name);
}

bool Core::ProcessVideo(CoreTask core_task) {
    if (core_task.target_paths.size() > 1) {
        logger_->warn(std::format("{}: Only one target video is supported. Only the first video is processed this time. \n{}", __FUNCTION__, core_task.target_paths.front()));
    }

    std::string& videoPath = core_task.target_paths.front();

    std::string audiosDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/audios");
    if (!core_task.skip_audio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(core_task.output_audio_encoder.value());
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            logger_->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        logger_->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    logger_->info(std::format("[Core] Extract Frames for {}", videoPath));
    std::string pattern = "frame_%06d." + core_task.temp_frame_format.value();
    std::string videoFramesOutputDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/" + ffc::FileSystem::getBaseName(videoPath));
    std::string outputPattern = videoFramesOutputDir + "/" + pattern;
    FfmpegRunner::extractFrames(videoPath, outputPattern);
    std::unordered_set<std::string> framePaths = FileSystem::listFilesInDir(videoFramesOutputDir);
    framePaths = FileSystem::filterImagePaths(framePaths);
    std::vector<std::string> framePathsVec(framePaths.begin(), framePaths.end());

    CoreTask tmpCoreRunOptions = core_task;
    tmpCoreRunOptions.target_paths = framePathsVec;
    tmpCoreRunOptions.output_paths = framePathsVec;
    ProcessImages(tmpCoreRunOptions);

    FfmpegRunner::VideoPrams videoPrams(videoPath);
    videoPrams.quality = core_task.output_video_quality.value();
    videoPrams.preset = core_task.output_video_preset.value();
    videoPrams.videoCodec = core_task.output_video_encoder.value();
    if (!framePathsVec.empty()) {
        cv::Mat firstFrame = vision::readStaticImage(framePathsVec[0]);
        videoPrams.width = firstFrame.cols;
        videoPrams.height = firstFrame.rows;
    }

    std::string inputImagePattern = videoFramesOutputDir + "/" + pattern;
    std::string outputVideo_NA_Path = FileSystem::parentPath(videoPath) + "/" + FileSystem::getBaseName(videoPath) + "_processed_NA" + ffc::FileSystem::getExtension(videoPath);
    Logger::getInstance()->info("[Core] Images to video : " + FileSystem::absolutePath(outputVideo_NA_Path));
    if (!FfmpegRunner::imagesToVideo(inputImagePattern, outputVideo_NA_Path, videoPrams)) {
        Logger::getInstance()->error("[Core] images to video failed!");
        FileSystem::removeDir(videoFramesOutputDir);
        FileSystem::removeFile(outputVideo_NA_Path);
        return false;
    }

    if (!core_task.skip_audio) {
        std::unordered_set<std::string> audioPaths = FileSystem::listFilesInDir(audiosDir);
        audioPaths = FfmpegRunner::filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::getInstance()->info("[Core] Add audios to video : " + FileSystem::absolutePath(core_task.output_paths.front()));
        if (!FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, core_task.output_paths.front())) {
            Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            FileSystem::moveFile(outputVideo_NA_Path, core_task.output_paths.front());
        } catch (const std::exception& e) {
            Logger::getInstance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    FileSystem::removeDir(videoFramesOutputDir);
    FileSystem::removeDir(audiosDir);
    FileSystem::removeFile(outputVideo_NA_Path);
    return true;
}

bool Core::ProcessVideoInSegments(CoreTask core_task) {
    std::string& videoPath = core_task.target_paths.front();

    std::string audiosDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/audios");
    if (!core_task.skip_audio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(core_task.output_audio_encoder.value());
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            logger_->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        logger_->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    std::string videoSegmentsDir = FileSystem::parentPath(videoPath) + "/videoSegments";
    std::string videoSegmentPattern = "segment_%03d" + FileSystem::getExtension(videoPath);
    Logger::getInstance()->info(std::format("[Core] Divide the video into segments of {} seconds each....", core_task.video_segment_duration.value()));
    if (!FfmpegRunner::cutVideoIntoSegments(videoPath, videoSegmentsDir,
                                            core_task.video_segment_duration.value(), videoSegmentPattern)) {
        Logger::getInstance()->error("The attempt to cut the video into segments was failed!");
        FileSystem::removeDir(audiosDir);
        return false;
    }

    std::unordered_set<std::string> videoSegmentsPathsSet = FileSystem::listFilesInDir(videoSegmentsDir);
    videoSegmentsPathsSet = FfmpegRunner::filterVideoPaths(videoSegmentsPathsSet);

    std::vector<std::string> processedVideoSegmentsPaths;
    std::string processedVideoSegmentsDir = FileSystem::parentPath(videoPath) + "/videoSegments_processed";
    std::vector<std::string> videoSegmentsPathsVec(videoSegmentsPathsSet.begin(), videoSegmentsPathsSet.end());
    std::ranges::sort(videoSegmentsPathsVec, [](const std::string& a, const std::string& b) {
        return a < b;
    });

    for (size_t segmentIndex = 0; segmentIndex < videoSegmentsPathsVec.size(); ++segmentIndex) {
        const auto& videoSegmentPath = videoSegmentsPathsVec.at(segmentIndex);
        std::string outputVideoSegmentPath = FileSystem::absolutePath(processedVideoSegmentsDir + "/" + ffc::FileSystem::getFileName(videoSegmentPath));

        CoreTask tmpCoreRunOptions = core_task;
        tmpCoreRunOptions.target_paths = {videoSegmentPath};
        tmpCoreRunOptions.output_paths = {outputVideoSegmentPath};
        tmpCoreRunOptions.skip_audio = true;

        logger_->info(std::format("[Core] Processing video segment {}/{}", segmentIndex + 1, videoSegmentsPathsSet.size()));
        if (!ProcessVideo(tmpCoreRunOptions)) {
            logger_->error(std::format("[Core] Failed to process video segment: {}", videoSegmentPath));
            return false;
        }
        processedVideoSegmentsPaths.emplace_back(outputVideoSegmentPath);

        FileSystem::removeFile(videoSegmentPath);
    }
    FileSystem::removeDir(videoSegmentsDir);

    FfmpegRunner::VideoPrams videoPrams(processedVideoSegmentsPaths[0]);
    videoPrams.quality = core_task.output_image_quality.value();
    videoPrams.preset = core_task.output_video_preset.value();
    videoPrams.videoCodec = core_task.output_video_encoder.value();

    std::string outputVideo_NA_Path = FileSystem::parentPath(videoPath) + "/" + ffc::FileSystem::getBaseName(videoPath) + "_processed_NA" + ffc::FileSystem::getExtension(videoPath);
    Logger::getInstance()->info("[Core] concat video segments...");
    if (!FfmpegRunner::concatVideoSegments(processedVideoSegmentsPaths, outputVideo_NA_Path, videoPrams)) {
        Logger::getInstance()->error("[Core] Failed concat video segments for : " + videoPath);
        FileSystem::removeDir(processedVideoSegmentsDir);
        return false;
    }

    if (!core_task.skip_audio) {
        std::unordered_set<std::string> audioPaths = FileSystem::listFilesInDir(audiosDir);
        audioPaths = FfmpegRunner::filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::getInstance()->info("[Core] Add audios to video...");
        if (!FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, core_task.output_paths.front())) {
            Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            FileSystem::moveFile(outputVideo_NA_Path, core_task.output_paths.front());
        } catch (const std::exception& e) {
            Logger::getInstance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    FileSystem::removeDir(audiosDir);
    FileSystem::removeFile(outputVideo_NA_Path);
    FileSystem::removeDir(processedVideoSegmentsDir);
    return true;
}

bool Core::ProcessVideos(const CoreTask& core_task, const bool& autoRemoveTarget) {
    if (core_task.target_paths.empty()) {
        logger_->error(" videoPaths is empty.");
        return false;
    }
    if (core_task.output_paths.empty()) {
        logger_->error("[Core::ProcessVideos] outputVideoPaths is empty.");
        return false;
    }
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        logger_->error("[Core::ProcessVideos] videoPaths and outputVideoPaths size mismatch.");
        return false;
    }

    bool isAllSuccess = true;
    for (size_t i = 0; i < core_task.target_paths.size(); ++i) {
        CoreTask tmpCoreRunOptions = core_task;
        tmpCoreRunOptions.target_paths = {core_task.target_paths[i]};
        tmpCoreRunOptions.output_paths = {core_task.output_paths[i]};
        logger_->info(std::format("Processing video {}/{}", i + 1, core_task.target_paths.size()));
        if (core_task.video_segment_duration.value() > 0) {
            if (ProcessVideoInSegments(tmpCoreRunOptions)) {
                logger_->info(std::format("[Core] Video processed successfully. Output path: {} ", core_task.output_paths[i]));
            } else {
                isAllSuccess = false;
                logger_->error(std::format("[Core] Video {} processed failed.", core_task.target_paths[i]));
            }
        } else {
            if (ProcessVideo(tmpCoreRunOptions)) {
                logger_->info(std::format("[Core] Video processed successfully. Output path: {} ", core_task.output_paths[i]));
            } else {
                isAllSuccess = false;
                logger_->error(std::format("[Core] Video {} processed failed.", core_task.target_paths[i]));
            }
        }
        if (autoRemoveTarget) {
            FileSystem::removeFile(core_task.target_paths[i]);
        }
    }
    return isAllSuccess;
}

bool Core::Run(CoreTask core_task) {
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        logger_->error("[Core::Run] target_paths and output_paths size mismatch.");
        return false;
    }

    if (core_task.processor_model.contains(ProcessorMajorType::FaceSwapper)
        || core_task.processor_model.contains(ProcessorMajorType::FaceEnhancer)
        || core_task.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (face_analyser_ == nullptr) {
            face_analyser_ = std::make_shared<FaceAnalyser>(env_, core_options_.inference_session_options);
        }
    }

    std::string tmpPath;
    do {
        std::string id = FileSystem::generateRandomString(10);
        tmpPath = FileSystem::getTempPath() + "/" + metadata::name + "/" + id;
        core_task.source_average_face_id = id;
    } while (FileSystem::dirExists(tmpPath));

    if (core_task.processor_minor_types.contains(ProcessorMajorType::FaceSwapper)
        && core_task.processor_minor_types.at(ProcessorMajorType::FaceSwapper).face_swapper.value() == FaceSwapperType::InSwapper) {
        std::unordered_set<std::string> sourceImgPaths(core_task.source_paths->begin(), core_task.source_paths->end());
        core_task.source_average_face_id = {FileSystem::hash::CombinedSHA1(sourceImgPaths)};
        core_task.source_average_face = std::make_shared<Face>(core_task.ProcessSourceAverageFace(face_analyser_));
    }

    std::vector<string> tmpTargetImgPaths, outputImgPaths;
    std::vector<string> tmpTargetVideoPaths, outputVideoPaths;
    for (size_t i = 0; i < core_task.target_paths.size(); ++i) {
        const std::string& targetPath = core_task.target_paths[i];
        const std::string& outputPath = core_task.output_paths[i];
        if (FileSystem::isImage(targetPath)) {
            std::string str = tmpPath + "/images/" + FileSystem::getFileName(targetPath);
            FileSystem::copy(targetPath, str);
            tmpTargetImgPaths.emplace_back(str);
            outputImgPaths.emplace_back(outputPath);
        } else if (FileSystem::isVideo(targetPath)) {
            const std::string file_symlink_path = tmpPath + "/videos/" + FileSystem::getFileName(targetPath);
            FileSystem::createDir(tmpPath + "/videos");
            try {
                std::filesystem::create_symlink(targetPath, file_symlink_path);
            } catch (std::filesystem::filesystem_error& e) {
                logger_->error(std::format("[Core::Run] Create symlink failed! Error: {}", e.what()));
            } catch (std::exception& e) {
                logger_->error(std::format("[Core::Run] Create symlink failed! Error: {}", e.what()));
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
        logger_->info("[Core] Processing Images...");
        if (!ProcessImages(tmpRunOptions)) {
            imagesAllOK = false;
        }
        FileSystem::moveFiles(tmpTargetImgPaths, outputImgPaths);
    }

    // process videos;
    bool videosAllOK = true;
    if (!tmpTargetVideoPaths.empty() && tmpTargetVideoPaths.size() == outputVideoPaths.size()) {
        CoreTask tmpRunOptions = core_task;
        tmpRunOptions.target_paths = tmpTargetVideoPaths;
        tmpRunOptions.output_paths = outputVideoPaths;
        logger_->info("[Core] Processing Videos...");
        if (!ProcessVideos(tmpRunOptions, true)) {
            videosAllOK = false;
        }
    }

    FileSystem::removeDir(tmpPath);
    return imagesAllOK && videosAllOK;
}

bool Core::ProcessImages(CoreTask core_task) {
    if (core_task.target_paths.empty()) {
        logger_->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (core_task.output_paths.empty()) {
        logger_->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }
    if (core_task.target_paths.size() != core_task.output_paths.size()) {
        logger_->error(std::string(__FUNCTION__) + " target_paths and output_paths size mismatch");
        return false;
    }

    if (core_task.processor_model.contains(ProcessorMajorType::FaceSwapper)
        && core_task.processor_minor_types[ProcessorMajorType::FaceSwapper].face_swapper.value() == FaceSwapperType::InSwapper) {
        if (core_task.source_average_face == nullptr) {
            core_task.source_average_face = std::make_shared<Face>(core_task.ProcessSourceAverageFace(face_analyser_));
        }
    }
    if (core_task.processor_list.front() == ProcessorMajorType::ExpressionRestorer) {
        if (!core_task.source_paths.has_value()) {
            logger_->error(std::string(__FUNCTION__) + " source_paths is nullopt");
        } else if (core_task.source_paths->empty()) {
            logger_->error(std::string(__FUNCTION__) + " source_paths is empty");
        } else if (core_task.target_paths.size() != core_task.source_paths->size()) {
            logger_->error(std::string(__FUNCTION__) + " target_paths and source_paths size mismatch");
        }
    }

    std::vector<std::string> originalTargetPaths;
    if (core_task.processor_minor_types.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (core_task.processor_list.front() != ProcessorMajorType::ExpressionRestorer) {
            for (const auto& path : core_task.target_paths) {
                const std::string tmpSourcePath = FileSystem::parentPath(path) + "/" + FileSystem::getBaseName(path) + "_original" + FileSystem::getExtension(path);
                originalTargetPaths.emplace_back(tmpSourcePath);
            }
            FileSystem::copyFiles(core_task.target_paths, originalTargetPaths);
        } else {
            if (!core_task.source_paths.has_value()) {
                core_task.source_paths = {{}};
            }
            originalTargetPaths = core_task.source_paths.value();
        }
    }

    for (auto itr = core_task.target_paths.begin(); itr != core_task.target_paths.end();) {
        if (!FileSystem::isImage(*itr)) {
            logger_->warn(std::string(__FUNCTION__) + " target_path is not image: " + *itr);
            itr = core_task.target_paths.erase(itr);
            continue;
        }
        ++itr;
    }

    std::unique_ptr<CoreTask> core_task_for_ER = nullptr;
    for (const auto& type : core_task.processor_list) {
        if (type == ProcessorMajorType::ExpressionRestorer) {
            if (originalTargetPaths.size() != core_task.target_paths.size()) {
                logger_->error(std::string(__FUNCTION__) + std::format(" source Paths size is {}, but target paths size is {}!", originalTargetPaths.size(), core_task.target_paths.size()));
                return false;
            }
            if (core_task_for_ER == nullptr) {
                core_task_for_ER = std::make_unique<CoreTask>(core_task);
                core_task_for_ER->source_paths = originalTargetPaths;
            }
        }

        auto func_to_process = [&](const size_t index) -> bool {
            if (type == ProcessorMajorType::FaceSwapper) {
                return SwapFace(core_task.GetFaceSwapperInput(index, face_analyser_),
                                core_task.output_paths.at(index),
                                core_task.processor_minor_types[type].face_swapper.value(),
                                core_task.processor_model[type]);
            }
            if (type == ProcessorMajorType::FaceEnhancer) {
                return EnhanceFace(core_task.GetFaceEnhancerInput(index, face_analyser_),
                                   core_task.output_paths.at(index),
                                   core_task.processor_minor_types[type].face_enhancer.value(),
                                   core_task.processor_model[type]);
            }
            if (type == ProcessorMajorType::ExpressionRestorer) {
                if (originalTargetPaths.empty()) {
                    logger_->error("ExpressionRestorer need source!");
                    return false;
                }
                CoreTask tmpCoreRunOptions = core_task;
                tmpCoreRunOptions.source_paths = originalTargetPaths;
                return RestoreExpression(tmpCoreRunOptions.GetExpressionRestorerInput(index, index, face_analyser_),
                                         core_task.output_paths.at(index),
                                         core_task.processor_minor_types[type].expression_restorer.value());
            }
            if (type == ProcessorMajorType::FrameEnhancer) {
                return EnhanceFrame(core_task.GetFrameEnhancerInput(index),
                                    core_task.output_paths.at(index),
                                    core_task.processor_minor_types[type].frame_enhancer.value(),
                                    core_task.processor_model[type]);
            }
            return false;
        };

        std::unique_ptr<ProgressBar> progress_bar{nullptr};
        std::string processor_name;
        if (core_task.show_progress_bar) {
            progress_bar = std::make_unique<ProgressBar>();
            if (type == ProcessorMajorType::FaceSwapper) {
                processor_name = processor_hub_.getProcessorPool().getFaceSwapper(core_task.processor_minor_types[type].face_swapper.value(), core_task.processor_model[type])->getProcessorName();
            }
            if (type == ProcessorMajorType::FaceEnhancer) {
                processor_name = processor_hub_.getProcessorPool().getFaceEnhancer(core_task.processor_minor_types[type].face_enhancer.value(), core_task.processor_model[type])->getProcessorName();
            }
            if (type == ProcessorMajorType::ExpressionRestorer) {
                processor_name = processor_hub_.getProcessorPool().getExpressionRestorer(core_task.processor_minor_types[type].expression_restorer.value())->getProcessorName();
            }
            if (type == ProcessorMajorType::FrameEnhancer) {
                processor_name = processor_hub_.getProcessorPool().getFrameEnhancer(core_task.processor_minor_types[type].frame_enhancer.value(), core_task.processor_model[type])->getProcessorName();
            }

            const size_t numTargetPaths = core_task.target_paths.size();
            ProgressBar::showConsoleCursor(false);
            progress_bar->setMaxProgress(100);
            progress_bar->setPrefixText(std::format("[{}] Processing ", processor_name));
            progress_bar->setPostfixText(std::format("{}/{}", 0, numTargetPaths));
            progress_bar->setProgress(0);
        }

        if (observer_) {
            observer_->onStart(core_task.target_paths.size());
        }

        std::vector<std::future<bool>> futures(core_task.target_paths.size());
        bool is_all_success = true;
        for (size_t index = 0; index < core_task.target_paths.size(); index += core_options_.execution_thread_count) {
            for (size_t j = index; j < index + core_options_.execution_thread_count && j < futures.size(); ++j) {
                futures.at(j) = ThreadPool::Instance()->Enqueue(func_to_process, j);
            }
            for (size_t k = index; k < index + core_options_.execution_thread_count && k < futures.size(); ++k) {
                while (!futures.at(k).valid()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                if (const auto is_success = futures[k].get(); !is_success) {
                    is_all_success = false;
                    logger_->error(std::format("[{}] Failed to write image: {}", processor_name, core_task.output_paths[k]));
                }

                if (core_task.show_progress_bar) {
                    progress_bar->setPostfixText(std::format("{}/{}", k + 1, futures.size()));
                    const int progress = static_cast<int>(std::floor(static_cast<float>(k + 1) * 100.0f) / static_cast<float>(futures.size()));
                    progress_bar->setProgress(progress);
                }

                if (observer_) {
                    observer_->onProgress(k + 1, std::format("Processing {}/{}", k + 1, futures.size()));
                }
            }
        }

        if (core_task.show_progress_bar) {
            ProgressBar::showConsoleCursor(true);
        }
        if (!is_all_success) {
            logger_->error(std::format("[{}] Some images failed to process or write.", processor_name));
        }

        if (observer_) {
            if (is_all_success) observer_->onComplete();
            else observer_->onError("Some images failed to process");
        }

        if (core_options_.processor_memory_strategy == CoreOptions::MemoryStrategy::Strict) {
            processor_hub_.getProcessorPool().removeProcessors(type);
        }
    }

    if (core_task.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        FileSystem::removeFiles(originalTargetPaths);
    }
    return true;
}

bool Core::SwapFace(const FaceSwapperInput& face_swapper_input, const std::string& output_path,
                    const FaceSwapperType& type, const ModelManager::Model& model) {
    if (output_path.empty()) {
        logger_->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat swapped_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FaceSwapperType::InSwapper) {
        target_frame = face_swapper_input.in_swapper_input->target_frame;
        swapped_frame = processor_hub_.swapFace(FaceSwapperType::InSwapper, model, face_swapper_input);
    }

    if (!swapped_frame.empty()) {
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, swapped_frame, output_path);
    } else {
        logger_->error(std::format("Swap face failed! Result frame is empty. And target_frame is {}!",
                                   target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, *target_frame, output_path);
    }

    return true;
}

bool Core::EnhanceFace(const FaceEnhancerInput& face_enhancer_input,
                       const std::string& output_path, const FaceEnhancerType& type,
                       const ModelManager::Model& model) {
    if (output_path.empty()) {
        logger_->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat enhanced_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FaceEnhancerType::CodeFormer) {
        target_frame = face_enhancer_input.code_former_input->target_frame;
        enhanced_frame = processor_hub_.enhanceFace(FaceEnhancerType::CodeFormer, model, face_enhancer_input);
    }
    if (type == FaceEnhancerType::GFP_GAN) {
        target_frame = face_enhancer_input.gfp_gan_input->target_frame;
        enhanced_frame = processor_hub_.enhanceFace(FaceEnhancerType::GFP_GAN, model, face_enhancer_input);
    }

    if (!enhanced_frame.empty()) {
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, enhanced_frame, output_path);
    } else {
        logger_->error(std::format("Enhance face failed! Result frame is empty. And target_frame is {}!",
                                   target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, *target_frame, output_path);
    }

    return true;
}

bool Core::RestoreExpression(const ExpressionRestorerInput& expression_restorer_input,
                             const std::string& output_path,
                             const ExpressionRestorerType& type) {
    if (output_path.empty()) {
        logger_->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat restored_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == ExpressionRestorerType::LivePortrait) {
        target_frame = expression_restorer_input.live_portrait_input->target_frame;
        restored_frame = processor_hub_.restoreExpression(ExpressionRestorerType::LivePortrait,
                                                          expression_restorer_input);
    }

    if (!restored_frame.empty()) {
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, restored_frame, output_path);
    } else {
        logger_->error(std::format("Restore expression failed! Result frame is empty. And target_frame is {}!",
                                   target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, *target_frame, output_path);
    }

    return true;
}

bool Core::EnhanceFrame(const FrameEnhancerInput& frame_enhancer_input,
                        const std::string& output_path, const FrameEnhancerType& type,
                        const ModelManager::Model& model) {
    if (output_path.empty()) {
        logger_->error(std::string(__FUNCTION__) + " output_path is empty");
        return false;
    }

    cv::Mat enhanced_frame;
    std::shared_ptr<cv::Mat> target_frame;
    if (type == FrameEnhancerType::Real_esr_gan) {
        target_frame = frame_enhancer_input.real_esr_gan_input->target_frame;
        enhanced_frame = processor_hub_.enhanceFrame(FrameEnhancerType::Real_esr_gan, model, frame_enhancer_input);
    }
    if (type == FrameEnhancerType::Real_hat_gan) {
        target_frame = frame_enhancer_input.real_hat_gan_input->target_frame;
        enhanced_frame = processor_hub_.enhanceFrame(FrameEnhancerType::Real_hat_gan, model, frame_enhancer_input);
    }

    if (!enhanced_frame.empty()) {
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, enhanced_frame, output_path);
    } else {
        logger_->error(std::format("Enhance frame failed! Result frame is empty. And target_frame is {}!",
                                   target_frame->empty() ? "empty" : "not empty"));
        std::ignore = ThreadPool::Instance()->Enqueue(vision::writeImage, *target_frame, output_path);
    }

    return true;
}

} // namespace ffc