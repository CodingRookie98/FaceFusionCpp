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
#include <thread_pool/thread_pool.h>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module core;
import :core_run_options;
import face_store;
import face_selector;
import vision;
import file_system;
import ffmpeg_runner;
import processor_hub;
import processor_hub;
import face_swapper;
import progress_bar;

using namespace std;
using namespace ffc;
using namespace faceSwapper;
using namespace faceEnhancer;
using namespace expressionRestore;
using namespace frameEnhancer;

namespace ffc {
Core::Core(const Options &options) :
    processor_hub_(options.inference_session_options) {
    coreOptions = options;
    Logger::getInstance()->setLogLevel(coreOptions.log_level);
    m_logger = Logger::getInstance();
    thread_pool_ = std::make_unique<dp::thread_pool<>>(options.execution_thread_count);

    if (coreOptions.force_download) {
        if (!ModelManager::getInstance()->downloadAllModel()) {
            m_logger->error("[Core] Download all model failed.");
            return;
        }
    }
}

Core::~Core() {
    FileSystem::removeDir(FileSystem::getTempPath());
}

bool Core::processVideo(CoreRunOptions _coreRunOptions) {
    if (_coreRunOptions.target_paths.size() > 1) {
        m_logger->warn(std::format("{}: Only one target video is supported. Only the first video is processed this time. \n{}", __FUNCTION__, _coreRunOptions.target_paths.front()));
    }

    std::string &videoPath = _coreRunOptions.target_paths.front();

    std::string audiosDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/audios");
    if (!_coreRunOptions.skip_audio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(_coreRunOptions.output_audio_encoder.value());
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    m_logger->info(std::format("[Core] Extract Frames for {}", videoPath));
    std::string pattern = "frame_%06d." + _coreRunOptions.temp_frame_format.value();
    std::string videoFramesOutputDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/" + ffc::FileSystem::getBaseName(videoPath));
    std::string outputPattern = videoFramesOutputDir + "/" + pattern;
    FfmpegRunner::extractFrames(videoPath, outputPattern);
    std::unordered_set<std::string> framePaths = FileSystem::listFilesInDir(videoFramesOutputDir);
    framePaths = FileSystem::filterImagePaths(framePaths);
    std::vector<std::string> framePathsVec(framePaths.begin(), framePaths.end());

    CoreRunOptions tmpCoreRunOptions = _coreRunOptions;
    tmpCoreRunOptions.target_paths = framePathsVec;
    tmpCoreRunOptions.output_paths = framePathsVec;
    processImages(tmpCoreRunOptions);

    FfmpegRunner::VideoPrams videoPrams(videoPath);
    videoPrams.quality = _coreRunOptions.output_video_quality.value();
    videoPrams.preset = _coreRunOptions.output_video_preset.value();
    videoPrams.videoCodec = _coreRunOptions.output_video_encoder.value();
    if (!framePathsVec.empty()) {
        cv::Mat firstFrame = Vision::readStaticImage(framePathsVec[0]);
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

    if (!_coreRunOptions.skip_audio) {
        std::unordered_set<std::string> audioPaths = FileSystem::listFilesInDir(audiosDir);
        audioPaths = FfmpegRunner::filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::getInstance()->info("[Core] Add audios to video : " + FileSystem::absolutePath(_coreRunOptions.output_paths.front()));
        if (!FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, _coreRunOptions.output_paths.front())) {
            Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            FileSystem::moveFile(outputVideo_NA_Path, _coreRunOptions.output_paths.front());
        } catch (const std::exception &e) {
            Logger::getInstance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    FileSystem::removeDir(videoFramesOutputDir);
    FileSystem::removeDir(audiosDir);
    FileSystem::removeFile(outputVideo_NA_Path);
    return true;
}

bool Core::processVideoInSegments(CoreRunOptions _coreRunOptions) {
    std::string &videoPath = _coreRunOptions.target_paths.front();

    std::string audiosDir = FileSystem::absolutePath(FileSystem::parentPath(videoPath) + "/audios");
    if (!_coreRunOptions.skip_audio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(_coreRunOptions.output_audio_encoder.value());
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    std::string videoSegmentsDir = FileSystem::parentPath(videoPath) + "/videoSegments";
    std::string videoSegmentPattern = "segment_%03d" + FileSystem::getExtension(videoPath);
    Logger::getInstance()->info(std::format("[Core] Divide the video into segments of {} seconds each....", _coreRunOptions.video_segment_duration.value()));
    if (!FfmpegRunner::cutVideoIntoSegments(videoPath, videoSegmentsDir,
                                            _coreRunOptions.video_segment_duration.value(), videoSegmentPattern)) {
        Logger::getInstance()->error("The attempt to cut the video into segments was failed!");
        FileSystem::removeDir(audiosDir);
        return false;
    }

    std::unordered_set<std::string> videoSegmentsPathsSet = FileSystem::listFilesInDir(videoSegmentsDir);
    videoSegmentsPathsSet = FfmpegRunner::filterVideoPaths(videoSegmentsPathsSet);

    std::vector<std::string> processedVideoSegmentsPaths;
    std::string processedVideoSegmentsDir = FileSystem::parentPath(videoPath) + "/videoSegments_processed";
    std::vector<std::string> videoSegmentsPathsVec(videoSegmentsPathsSet.begin(), videoSegmentsPathsSet.end());
    std::ranges::sort(videoSegmentsPathsVec, [](const std::string &a, const std::string &b) {
        return a < b;
    });

    for (size_t segmentIndex = 0; segmentIndex < videoSegmentsPathsVec.size(); ++segmentIndex) {
        const auto &videoSegmentPath = videoSegmentsPathsVec.at(segmentIndex);
        std::string outputVideoSegmentPath = FileSystem::absolutePath(processedVideoSegmentsDir + "/" + ffc::FileSystem::getFileName(videoSegmentPath));

        CoreRunOptions tmpCoreRunOptions = _coreRunOptions;
        tmpCoreRunOptions.target_paths = {videoSegmentPath};
        tmpCoreRunOptions.output_paths = {outputVideoSegmentPath};
        tmpCoreRunOptions.skip_audio = true;

        m_logger->info(std::format("[Core] Processing video segment {}/{}", segmentIndex + 1, videoSegmentsPathsSet.size()));
        if (!processVideo(tmpCoreRunOptions)) {
            m_logger->error(std::format("[Core] Failed to process video segment: {}", videoSegmentPath));
            return false;
        }
        processedVideoSegmentsPaths.emplace_back(outputVideoSegmentPath);

        FileSystem::removeFile(videoSegmentPath);
    }
    FileSystem::removeDir(videoSegmentsDir);

    FfmpegRunner::VideoPrams videoPrams(processedVideoSegmentsPaths[0]);
    videoPrams.quality = _coreRunOptions.output_image_quality.value();
    videoPrams.preset = _coreRunOptions.output_video_preset.value();
    videoPrams.videoCodec = _coreRunOptions.output_video_encoder.value();

    std::string outputVideo_NA_Path = FileSystem::parentPath(videoPath) + "/" + ffc::FileSystem::getBaseName(videoPath) + "_processed_NA" + ffc::FileSystem::getExtension(videoPath);
    Logger::getInstance()->info("[Core] concat video segments...");
    if (!FfmpegRunner::concatVideoSegments(processedVideoSegmentsPaths, outputVideo_NA_Path, videoPrams)) {
        Logger::getInstance()->error("[Core] Failed concat video segments for : " + videoPath);
        FileSystem::removeDir(processedVideoSegmentsDir);
        return false;
    }

    if (!_coreRunOptions.skip_audio) {
        std::unordered_set<std::string> audioPaths = FileSystem::listFilesInDir(audiosDir);
        audioPaths = FfmpegRunner::filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Logger::getInstance()->info("[Core] Add audios to video...");
        if (!FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, _coreRunOptions.output_paths.front())) {
            Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            FileSystem::moveFile(outputVideo_NA_Path, _coreRunOptions.output_paths.front());
        } catch (const std::exception &e) {
            Logger::getInstance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    FileSystem::removeDir(audiosDir);
    FileSystem::removeFile(outputVideo_NA_Path);
    FileSystem::removeDir(processedVideoSegmentsDir);
    return true;
}

bool Core::processVideos(CoreRunOptions _coreRunOptions) {
    if (_coreRunOptions.target_paths.empty()) {
        m_logger->error(" videoPaths is empty.");
        return false;
    }
    if (_coreRunOptions.output_paths.empty()) {
        m_logger->error("[Core::ProcessVideos] outputVideoPaths is empty.");
        return false;
    }
    if (_coreRunOptions.target_paths.size() != _coreRunOptions.output_paths.size()) {
        m_logger->error("[Core::ProcessVideos] videoPaths and outputVideoPaths size mismatch.");
        return false;
    }

    bool isAllSuccess = true;
    for (size_t i = 0; i < _coreRunOptions.target_paths.size(); ++i) {
        CoreRunOptions tmpCoreRunOptions = _coreRunOptions;
        tmpCoreRunOptions.target_paths = {_coreRunOptions.target_paths[i]};
        tmpCoreRunOptions.output_paths = {_coreRunOptions.output_paths[i]};
        if (_coreRunOptions.video_segment_duration.value() > 0) {
            if (processVideoInSegments(tmpCoreRunOptions)) {
                m_logger->info(std::format("[Core] Video processed successfully. Output path: {} ", _coreRunOptions.output_paths[i]));
            } else {
                isAllSuccess = false;
                m_logger->error(std::format("[Core] Video {} processed failed.", _coreRunOptions.target_paths[i]));
            }
        } else {
            if (processVideo(tmpCoreRunOptions)) {
                m_logger->info(std::format("[Core] Video processed successfully. Output path: {} ", _coreRunOptions.output_paths[i]));
            } else {
                isAllSuccess = false;
                m_logger->error(std::format("[Core] Video {} processed failed.", _coreRunOptions.target_paths[i]));
            }
        }
    }
    return isAllSuccess;
}

bool Core::run(CoreRunOptions _coreRunOptions) {
    if (_coreRunOptions.target_paths.size() != _coreRunOptions.output_paths.size()) {
        m_logger->error("[Core::Run] target_paths and output_paths size mismatch.");
        return false;
    }

    if (_coreRunOptions.processor_model.contains(ProcessorMajorType::FaceSwapper)
        || _coreRunOptions.processor_model.contains(ProcessorMajorType::FaceEnhancer)
        || _coreRunOptions.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (m_faceAnalyser == nullptr) {
            m_faceAnalyser = std::make_shared<FaceAnalyser>(m_env, coreOptions.inference_session_options);
        }
    }

    std::string tmpPath;
    do {
        std::string id = FileSystem::generateRandomString(10);
        tmpPath = FileSystem::getTempPath() + "/" + id;
        _coreRunOptions.source_average_face_id = id;
    } while (FileSystem::dirExists(tmpPath));

    std::vector<string> targetImgPaths, tmpTargetImgPaths, outputImgPaths;
    std::vector<string> targetVideoPaths, tmpTargetVideoPaths, outputVideoPaths;
    for (size_t i = 0; i < _coreRunOptions.target_paths.size(); ++i) {
        std::string targetPath = _coreRunOptions.target_paths[i];
        std::string outputPath = _coreRunOptions.output_paths[i];
        if (FileSystem::isImage(targetPath)) {
            std::string str = tmpPath + "/images/" + FileSystem::getFileName(targetPath);
            FileSystem::copy(targetPath, str);
            targetImgPaths.emplace_back(targetPath);
            tmpTargetImgPaths.emplace_back(str);
            outputImgPaths.emplace_back(outputPath);
        }
        if (FileSystem::isVideo(targetPath)) {
            std::string str = tmpPath + "/videos/" + FileSystem::getFileName(targetPath);
            FileSystem::copy(targetPath, str);
            targetVideoPaths.emplace_back(targetPath);
            tmpTargetVideoPaths.emplace_back(str);
            outputVideoPaths.emplace_back(outputPath);
        }
    }

    // process images;
    bool imagesAllOK = true;
    if (!tmpTargetImgPaths.empty() && tmpTargetImgPaths.size() == outputImgPaths.size()) {
        CoreRunOptions tmpRunOptions = _coreRunOptions;
        tmpRunOptions.target_paths = tmpTargetImgPaths;
        tmpRunOptions.output_paths = tmpTargetImgPaths;
        if (!processImages(tmpRunOptions)) {
            imagesAllOK = false;
        }
        FileSystem::moveFiles(tmpTargetImgPaths, outputImgPaths, std::thread::hardware_concurrency());
    }

    // process videos;
    bool videosAllOK = true;
    if (!tmpTargetVideoPaths.empty() && tmpTargetVideoPaths.size() == outputVideoPaths.size()) {
        CoreRunOptions tmpRunOptions = _coreRunOptions;
        tmpRunOptions.target_paths = tmpTargetVideoPaths;
        tmpRunOptions.output_paths = outputVideoPaths;
        if (!processVideos(tmpRunOptions)) {
            videosAllOK = false;
        }
    }

    FileSystem::removeDir(tmpPath);
    return imagesAllOK && videosAllOK;
}

bool Core::processImages(CoreRunOptions _coreRunOptions) {
    if (_coreRunOptions.target_paths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (_coreRunOptions.output_paths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }
    if (_coreRunOptions.target_paths.size() != _coreRunOptions.output_paths.size()) {
        m_logger->error(std::string(__FUNCTION__) + " target_paths and output_paths size mismatch");
        return false;
    }

    if (_coreRunOptions.processor_model.contains(ProcessorMajorType::FaceSwapper)
        && _coreRunOptions.processor_minor_types[ProcessorMajorType::FaceSwapper].face_swapper.value() == FaceSwapperType::InSwapper) {
        if (m_faceAnalyser->getFaceStore()->getFaces(_coreRunOptions.source_average_face_id.value()).empty()) {
            if (!processSourceAverageFace(_coreRunOptions)) {
                m_logger->error(std::string(__FUNCTION__) + " processSourceAverageFace failed");
                return false;
            }
        }
    }
    if (_coreRunOptions.processor_list.front() == ProcessorMajorType::ExpressionRestorer) {
        if (!_coreRunOptions.source_paths.has_value()) {
            m_logger->error(std::string(__FUNCTION__) + " source_paths is nullopt");
        } else if (_coreRunOptions.source_paths->empty()) {
            m_logger->error(std::string(__FUNCTION__) + " source_paths is empty");
        } else if (_coreRunOptions.target_paths.size() != _coreRunOptions.source_paths->size()) {
            m_logger->error(std::string(__FUNCTION__) + " target_paths and source_paths size mismatch");
        }
    }

    std::vector<std::string> originalTargetPaths;
    if (_coreRunOptions.processor_minor_types.contains(ProcessorMajorType::ExpressionRestorer)) {
        if (_coreRunOptions.processor_list.front() != ProcessorMajorType::ExpressionRestorer) {
            for (const auto &path : _coreRunOptions.target_paths) {
                const std::string tmpSourcePath = FileSystem::parentPath(path) + "/" + FileSystem::getBaseName(path) + "_original" + FileSystem::getExtension(path);
                originalTargetPaths.emplace_back(tmpSourcePath);
            }
            FileSystem::copyFiles(_coreRunOptions.target_paths, originalTargetPaths);
        } else {
            if (!_coreRunOptions.source_paths.has_value()) {
                _coreRunOptions.source_paths = {{}};
            }
            originalTargetPaths = _coreRunOptions.source_paths.value();
        }
    }

    for (auto itr = _coreRunOptions.target_paths.begin(); itr != _coreRunOptions.target_paths.end();) {
        if (!FileSystem::isImage(*itr)) {
            m_logger->warn(std::string(__FUNCTION__) + " target_path is not image: " + *itr);
            itr = _coreRunOptions.target_paths.erase(itr);
            continue;
        }
        ++itr;
    }

    for (const auto &type : _coreRunOptions.processor_list) {
        if (type == ProcessorMajorType::ExpressionRestorer) {
            if (originalTargetPaths.size() != _coreRunOptions.target_paths.size()) {
                m_logger->error(std::string(__FUNCTION__) + std::format(" source Paths size is {}, but target paths size is {}!", originalTargetPaths.size(), _coreRunOptions.target_paths.size()));
                return false;
            }
        }

        std::vector<std::future<bool>> futures;

        for (size_t i = 0; i < _coreRunOptions.target_paths.size(); ++i) {
            auto func = [&](const size_t index) -> bool {
                CoreRunOptions tmpCoreRunOptions = _coreRunOptions;
                tmpCoreRunOptions.target_paths.clear();
                tmpCoreRunOptions.output_paths.clear();
                std::string targetPath = _coreRunOptions.target_paths[index];
                std::string outputPath = _coreRunOptions.output_paths[index];
                tmpCoreRunOptions.target_paths.emplace_back(targetPath);
                tmpCoreRunOptions.output_paths.emplace_back(outputPath);

                if (type == ProcessorMajorType::FaceSwapper) {
                    return swapFace(tmpCoreRunOptions);
                }
                if (type == ProcessorMajorType::FaceEnhancer) {
                    return enhanceFace(tmpCoreRunOptions);
                }
                if (type == ProcessorMajorType::ExpressionRestorer) {
                    if (originalTargetPaths.empty()) {
                        m_logger->error("ExpressionRestorer need source!");
                        return false;
                    }
                    if (!tmpCoreRunOptions.source_paths.has_value()) {
                        tmpCoreRunOptions.source_paths = {{}};
                    }
                    tmpCoreRunOptions.source_paths->clear();
                    tmpCoreRunOptions.source_paths->emplace_back(originalTargetPaths.at(index));
                    return restoreExpression(tmpCoreRunOptions);
                }
                if (type == ProcessorMajorType::FrameEnhancer) {
                    return enhanceFrame(tmpCoreRunOptions);
                }
                return false;
            };
            futures.emplace_back(thread_pool_->enqueue(func, i));
        }

        if (_coreRunOptions.show_progress_bar) {
            std::string processorName;
            if (type == ProcessorMajorType::FaceSwapper) {
                processorName = processor_hub_.getProcessorPool().getFaceSwapper(_coreRunOptions.processor_minor_types[type].face_swapper.value(), _coreRunOptions.processor_model[type])->getProcessorName();
            }
            if (type == ProcessorMajorType::FaceEnhancer) {
                processorName = processor_hub_.getProcessorPool().getFaceEnhancer(_coreRunOptions.processor_minor_types[type].face_enhancer.value(), _coreRunOptions.processor_model[type])->getProcessorName();
            }
            if (type == ProcessorMajorType::ExpressionRestorer) {
                processorName = processor_hub_.getProcessorPool().getExpressionRestorer(_coreRunOptions.processor_minor_types[type].expression_restorer.value())->getProcessorName();
            }
            if (type == ProcessorMajorType::FrameEnhancer) {
                processorName = processor_hub_.getProcessorPool().getFrameEnhancer(_coreRunOptions.processor_minor_types[type].frame_enhancer.value(), _coreRunOptions.processor_model[type])->getProcessorName();
            }

            const size_t numTargetPaths = _coreRunOptions.target_paths.size();
            ProgressBar::showConsoleCursor(false);
            ProgressBar bar;
            bar.setMaxProgress(100);
            bar.setPrefixText(std::format("[{}] Processing ", processorName));
            bar.setPostfixText(std::format("{}/{}", 0, numTargetPaths));
            bar.setProgress(0);
            int i = 0;
            bool isAllWriteSuccess = true;
            while (true) {
                if (futures.size() <= i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                if (!futures[i].valid()) {
                    isAllWriteSuccess = false;
                    m_logger->error(std::format("[{}] Failed to process image: {}", processorName, _coreRunOptions.target_paths[i]));
                    ++i;
                    continue;
                }

                if (const auto writeIsSuccess = futures[i].get(); !writeIsSuccess) {
                    isAllWriteSuccess = false;
                    m_logger->error(std::format("[{}] Failed to write image: {}", processorName, _coreRunOptions.output_paths[i]));
                }

                bar.setPostfixText(std::format("{}/{}", i + 1, numTargetPaths));
                const int progress = static_cast<int>(std::floor(static_cast<float>(i + 1) * 100.0f) / static_cast<float>(numTargetPaths));
                bar.setProgress(progress);

                ++i;
                if (i >= _coreRunOptions.output_paths.size()) {
                    break;
                }
            }
            ProgressBar::showConsoleCursor(true);
            if (!isAllWriteSuccess) {
                m_logger->error(std::format("[{}] Some images failed to process or write.", processorName));
            }
        }

        if (coreOptions.processor_memory_strategy == Options::MemoryStrategy::Strict) {
            processor_hub_.getProcessorPool().removeProcessors(type);
        }
    }

    if (_coreRunOptions.processor_model.contains(ProcessorMajorType::ExpressionRestorer)) {
        FileSystem::removeFiles(originalTargetPaths, std::thread::hardware_concurrency() / 2);
    }
    return true;
}

bool Core::processSourceAverageFace(CoreRunOptions _coreRunOptions) const {
    // process source_average_face
    if (!_coreRunOptions.source_paths.has_value()) {
        m_logger->error(std::string(__FUNCTION__) + " source_paths is empty");
        return false;
    }
    std::vector<std::string> sourceImagePaths = _coreRunOptions.source_paths.value();
    if (sourceImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " source_path is empty");
        return false;
    }

    std::unordered_set<std::string> sourcePaths(sourceImagePaths.cbegin(), sourceImagePaths.cend());
    sourcePaths = FileSystem::filterImagePaths(sourcePaths);
    if (sourcePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + "The source path is a directory but the directory does not contain any image files!");
        return false;
    }

    if (!m_faceAnalyser->getFaceStore()->getFaces(_coreRunOptions.source_average_face_id.value()).empty()) {
        return true;
    }

    Face sourceAverageFace;
    sourcePaths = FileSystem::filterImagePaths(sourcePaths);
    if (!_coreRunOptions.face_analyser_options.has_value()) {
        m_logger->warn(std::string(__FUNCTION__) + " face_analyser_options is empty");
        _coreRunOptions.face_analyser_options = FaceAnalyser::Options{};
    }

    std::vector<cv::Mat> frames = Vision::readStaticImages(sourcePaths, std::thread::hardware_concurrency() / 2);
    sourceAverageFace = m_faceAnalyser->getAverageFace(frames, _coreRunOptions.face_analyser_options.value());
    frames.clear();
    if (sourceAverageFace.isEmpty()) {
        m_logger->error(std::string(__FUNCTION__) + " source face is empty");
        return false;
    }
    m_faceAnalyser->getFaceStore()->appendFaces(_coreRunOptions.source_average_face_id.value(), {sourceAverageFace});
    return true;
}

bool Core::swapFace(CoreRunOptions _coreRunOptions) {
    std::vector<std::string> &targetImagePaths = _coreRunOptions.target_paths;
    std::vector<std::string> &outputImagePaths = _coreRunOptions.output_paths;

    if (targetImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (outputImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }

    for (size_t i = 0; i < targetImagePaths.size(); ++i) {
        cv::Mat targetFrame = Vision::readStaticImage(targetImagePaths[i]);
        std::vector<Face> targetFaces = getTargetFaces(_coreRunOptions, targetFrame);

        if (targetFaces.empty()) {
            Vision::writeImage(targetFrame, outputImagePaths[i]);
            continue;
        }
        cv::Mat swappedFrame;
        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::FaceSwapper].face_swapper.value()
            == FaceSwapperType::InSwapper) {
            FaceSwapperInput faceSwapperInput;
            faceSwapperInput.in_swapper_input = std::make_optional(InSwapperInput{});

            faceSwapperInput.in_swapper_input->targetFrame = &targetFrame;
            Face sourceAverageFace = m_faceAnalyser->getFaceStore()->getFaces(_coreRunOptions.source_average_face_id.value()).front();
            faceSwapperInput.in_swapper_input->sourceFace = &sourceAverageFace;
            faceSwapperInput.in_swapper_input->targetFaces = &targetFaces;
            faceSwapperInput.in_swapper_input->boxMaskBlur = _coreRunOptions.face_mask_blur.value();
            faceSwapperInput.in_swapper_input->boxMaskPadding = _coreRunOptions.face_mask_padding.value();
            faceSwapperInput.in_swapper_input->faceMaskersTypes = _coreRunOptions.face_mask_types.value();
            faceSwapperInput.in_swapper_input->faceMaskerRegions = _coreRunOptions.face_mask_regions.value();

            swappedFrame = processor_hub_.swapFace(FaceSwapperType::InSwapper,
                                                   _coreRunOptions.processor_model[ProcessorMajorType::FaceSwapper],
                                                   faceSwapperInput);
        }

        if (!swappedFrame.empty()) {
            Vision::writeImage(swappedFrame, outputImagePaths[i]);
        } else {
            m_logger->error(std::format("Swap face failed for {}", targetImagePaths[i]));
            Vision::writeImage(targetFrame, outputImagePaths[i]);
        }
        targetFrame.release();
    }

    return true;
}

bool Core::enhanceFace(CoreRunOptions _coreRunOptions) {
    std::vector<std::string> &targetImagePaths = _coreRunOptions.target_paths;
    std::vector<std::string> &outputImagePaths = _coreRunOptions.output_paths;

    if (targetImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (outputImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }

    for (size_t i = 0; i < targetImagePaths.size(); ++i) {
        cv::Mat targetFrame = Vision::readStaticImage(targetImagePaths[i]);
        std::vector<Face> targetFaces = getTargetFaces(_coreRunOptions, targetFrame);

        if (targetFaces.empty()) {
            Vision::writeImage(targetFrame, outputImagePaths[i]);
            continue;
        }

        cv::Mat enhancedFrame;
        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::FaceEnhancer].face_enhancer.value()
            == FaceEnhancerType::CodeFormer) {
            FaceEnhancerInput faceEnhancerInput;
            faceEnhancerInput.code_former_input = std::make_optional(CodeFormerInput{});
            faceEnhancerInput.code_former_input->targetFrame = &targetFrame;
            faceEnhancerInput.code_former_input->targetFaces = &targetFaces;
            faceEnhancerInput.code_former_input->faceBlend = _coreRunOptions.face_enhancer_blend.value();
            faceEnhancerInput.code_former_input->faceMaskersTypes = _coreRunOptions.face_mask_types.value();
            faceEnhancerInput.code_former_input->boxMaskBlur = _coreRunOptions.face_mask_blur.value();
            faceEnhancerInput.code_former_input->boxMaskPadding = _coreRunOptions.face_mask_padding.value();

            enhancedFrame = processor_hub_.enhanceFace(FaceEnhancerType::CodeFormer,
                                                       _coreRunOptions.processor_model[ProcessorMajorType::FaceEnhancer],
                                                       faceEnhancerInput);
        }

        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::FaceEnhancer].face_enhancer.value()
            == FaceEnhancerType::GFP_GAN) {
            FaceEnhancerInput faceEnhancerInput;
            faceEnhancerInput.gfp_gan_input = std::make_optional(GFP_GAN_Input{});
            faceEnhancerInput.gfp_gan_input->targetFrame = &targetFrame;
            faceEnhancerInput.gfp_gan_input->targetFaces = &targetFaces;
            faceEnhancerInput.gfp_gan_input->faceBlend = _coreRunOptions.face_enhancer_blend.value();
            faceEnhancerInput.gfp_gan_input->faceMaskersTypes = _coreRunOptions.face_mask_types.value();
            faceEnhancerInput.gfp_gan_input->boxMaskBlur = _coreRunOptions.face_mask_blur.value();
            faceEnhancerInput.gfp_gan_input->boxMaskPadding = _coreRunOptions.face_mask_padding.value();

            enhancedFrame = processor_hub_.enhanceFace(FaceEnhancerType::GFP_GAN,
                                                       _coreRunOptions.processor_model[ProcessorMajorType::FaceEnhancer],
                                                       faceEnhancerInput);
        }

        if (!enhancedFrame.empty()) {
            Vision::writeImage(enhancedFrame, outputImagePaths[i]);
        } else {
            m_logger->error(std::format("Enhance face failed for {}", targetImagePaths[i]));
            Vision::writeImage(targetFrame, outputImagePaths[i]);
        }
        targetFrame.release();
    }

    return true;
}

bool Core::restoreExpression(CoreRunOptions _coreRunOptions) {
    std::vector<std::string> &targetImagePaths = _coreRunOptions.target_paths;
    std::vector<std::string> &sourceImagePaths = _coreRunOptions.source_paths.value();
    std::vector<std::string> &outputImagePaths = _coreRunOptions.output_paths;

    if (targetImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (sourceImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " sourceImagePaths is empty");
        return false;
    }
    if (outputImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }
    if (sourceImagePaths.size() != targetImagePaths.size() || targetImagePaths.size() != outputImagePaths.size()) {
        m_logger->error(std::string(__FUNCTION__) + " The sizes of two of the three do not match, sourceImagePaths.size()、 targetImagePaths.size()、 outputImagePaths.size()");
        return false;
    }

    for (size_t i = 0; i < targetImagePaths.size(); ++i) {
        cv::Mat targetFrame = Vision::readStaticImage(targetImagePaths[i]);
        cv::Mat sourceFrame = Vision::readStaticImage(sourceImagePaths[i]);
        std::vector<Face> targetFaces = getTargetFaces(_coreRunOptions, targetFrame);

        if (targetFaces.empty()) {
            Vision::writeImage(targetFrame, outputImagePaths[i]);
            continue;
        }

        cv::Mat restoredFrame;
        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::ExpressionRestorer].expression_restorer.value()
            == ExpressionRestorerType::LivePortrait) {
            ExpressionRestorerInput expression_restorer_input;
            expression_restorer_input.live_portrait_input = std::make_optional(LivePortraitInput{});
            expression_restorer_input.live_portrait_input->targetFrame = &targetFrame;
            expression_restorer_input.live_portrait_input->sourceFrame = &sourceFrame;
            expression_restorer_input.live_portrait_input->targetFaces = &targetFaces;
            expression_restorer_input.live_portrait_input->restoreFactor = _coreRunOptions.face_enhancer_blend.value();
            expression_restorer_input.live_portrait_input->faceMaskersTypes = _coreRunOptions.face_mask_types.value();
            expression_restorer_input.live_portrait_input->boxMaskBlur = _coreRunOptions.face_mask_blur.value();
            expression_restorer_input.live_portrait_input->boxMaskPadding = _coreRunOptions.face_mask_padding.value();

            restoredFrame = processor_hub_.restoreExpression(ExpressionRestorerType::LivePortrait,
                                                             expression_restorer_input);
        }
        if (!restoredFrame.empty()) {
            Vision::writeImage(restoredFrame, outputImagePaths[i]);
        } else {
            m_logger->error(std::format("Restore expression failed for {}", targetImagePaths[i]));
            Vision::writeImage(targetFrame, outputImagePaths[i]);
        }
        targetFrame.release();
        restoredFrame.release();
    }
    return true;
}

bool Core::enhanceFrame(CoreRunOptions _coreRunOptions) {
    std::vector<std::string> &targetImagePaths = _coreRunOptions.target_paths;
    const std::vector<std::string> &outputImagePaths = _coreRunOptions.output_paths;

    if (targetImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " targetImagePaths is empty");
        return false;
    }
    if (outputImagePaths.empty()) {
        m_logger->error(std::string(__FUNCTION__) + " outputImagePaths is empty");
        return false;
    }
    for (size_t i = 0; i < targetImagePaths.size(); ++i) {
        cv::Mat targetFrame = Vision::readStaticImage(targetImagePaths[i]);

        cv::Mat enhancedFrame;
        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::FrameEnhancer].frame_enhancer.value()
            == FrameEnhancerType::Real_esr_gan) {
            FrameEnhancerInput frame_enhancer_input;
            frame_enhancer_input.real_esr_gan_input = std::make_optional(RealEsrGanInput{});
            frame_enhancer_input.real_esr_gan_input->targetFrame = &targetFrame;
            frame_enhancer_input.real_esr_gan_input->blend = _coreRunOptions.frame_enhancer_blend.value();

            enhancedFrame = processor_hub_.enhanceFrame(FrameEnhancerType::Real_esr_gan,
                                                        _coreRunOptions.processor_model[ProcessorMajorType::FrameEnhancer],
                                                        frame_enhancer_input);
        }
        if (_coreRunOptions.processor_minor_types[ProcessorMajorType::FrameEnhancer].frame_enhancer.value()
            == FrameEnhancerType::Real_hat_gan) {
            FrameEnhancerInput frame_enhancer_input;
            frame_enhancer_input.real_hat_gan_input = std::make_optional(RealHatGanInput{});
            frame_enhancer_input.real_hat_gan_input->targetFrame = &targetFrame;
            frame_enhancer_input.real_hat_gan_input->blend = _coreRunOptions.frame_enhancer_blend.value();

            enhancedFrame = processor_hub_.enhanceFrame(FrameEnhancerType::Real_hat_gan,
                                                        _coreRunOptions.processor_model[ProcessorMajorType::FrameEnhancer],
                                                        frame_enhancer_input);
        }

        targetFrame.release();
        if (!enhancedFrame.empty()) {
            Vision::writeImage(enhancedFrame, outputImagePaths[i]);
        } else {
            m_logger->error(std::format("Swap face failed for {}", targetImagePaths[i]));
            Vision::writeImage(targetFrame, outputImagePaths[i]);
        }
        enhancedFrame.release();
    }
    return true;
}

std::vector<Face> Core::getTargetFaces(const CoreRunOptions &_coreRunOptions,
                                       const cv::Mat &targetFrame) const {
    std::vector<Face> targetFaces;
    if (_coreRunOptions.face_selector_mode.value() == FaceSelector::SelectorMode::Many) {
        targetFaces = m_faceAnalyser->getManyFaces(targetFrame, _coreRunOptions.face_analyser_options.value());
    } else if (_coreRunOptions.face_selector_mode.value() == FaceSelector::SelectorMode::One) {
        targetFaces.emplace_back(m_faceAnalyser->getOneFace(targetFrame, _coreRunOptions.face_analyser_options.value(), _coreRunOptions.reference_face_position.value()));
    } else {
        if (_coreRunOptions.reference_face_path.value().empty()) {
            m_logger->error(std::string(__FUNCTION__) + " reference_face_path is empty");
            return {};
        }
        if (FileSystem::isImage(_coreRunOptions.reference_face_path.value())) {
            const cv::Mat refFrame = Vision::readStaticImage(_coreRunOptions.reference_face_path.value());
            const vector<Face> refFaces = m_faceAnalyser->getManyFaces(refFrame, _coreRunOptions.face_analyser_options.value());
            if (refFaces.empty()) {
                m_logger->error(std::string(__FUNCTION__) + " reference_face is empty");
                return {};
            }
            const vector<Face> simFaces = m_faceAnalyser->findSimilarFaces(refFaces, targetFrame, _coreRunOptions.reference_face_distance.value(), _coreRunOptions.face_analyser_options.value());
            if (!simFaces.empty()) {
                targetFaces = simFaces;
            } else {
                m_logger->error(std::string(__FUNCTION__) + " reference_face is empty");
            }

        } else {
            m_logger->error(std::string(__FUNCTION__) + " reference_face_path is not a image file");
            return {};
        }
    }
    return targetFaces;
}

} // namespace ffc