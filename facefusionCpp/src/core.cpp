/**
 ******************************************************************************
 * @file           : core.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

#include "core.h"
#include <chrono>
#include <thread_pool/thread_pool.h>
#include "file_system.h"
#include "ffmpeg_runner.h"
#include "model_manager.h"
#include "face_selector.h"
#include "face_swapper_helper.h"
#include "face_swapper_base.h"
#include "face_enhancer_helper.h"
#include "metadata.h"
#include "progress_bar.h"
#include "vision.h"
#include "expression_restorer.h"
#include "frame_enhancer_helper.h"

namespace Ffc {
Core::Core() {
    m_env = std::make_shared<Ort::Env>(Ort::Env(ORT_LOGGING_LEVEL_ERROR, "faceFusionCpp"));
    m_config = Config::getInstance();
    m_logger = Logger::getInstance();

    m_logger->setLogLevel(m_config->m_logLevel);
    m_logger->info(MetaData::name + " version: " + MetaData::version + " " + MetaData::url);
    m_logger->info(std::format("onnxruntime version: {}", Ort::GetVersionString()));
    
    if (m_config->m_forceDownload) {
        if (!ModelManager::getInstance()->downloadAllModel()) {
            m_logger->error("[Core::run] Download all model failed.");
            return;
        }
    }
    
    if (m_config->m_frameProcessors.size() == 1 && m_config->m_frameProcessors[0] == ProcessorBase::FrameEnhancer) {
        // 如果只有frameEnhancer, 不需要初始化以下两项
        m_faceAnalyser = nullptr;
        m_faceMaskers = nullptr;
    } else {
        m_faceAnalyser = std::make_shared<FaceAnalyser>(m_env, m_config);
        m_faceMaskers = std::make_shared<FaceMaskers>(m_env);
        m_faceMaskers->setFaceMaskPadding(m_config->m_faceMaskPadding);
        m_faceMaskers->setFaceMaskBlur(m_config->m_faceMaskBlur);
        m_faceMaskers->setFaceMaskRegions(m_config->m_faceMaskRegionsSet);
    }
    
    for (const auto &type : m_config->m_frameProcessors) {
        if (!m_processorPtrMap.contains(type)) {
            if (m_config->m_processorMemoryStrategy == Config::ProcessorMemoryStrategy::Tolerant) {
                m_processorPtrMap[type] = createProcessor(type);
            } else if (m_config->m_processorMemoryStrategy == Config::ProcessorMemoryStrategy::Strict) {
                m_processorPtrMap[type] = nullptr;
            }
        }
    }
}

Core::~Core() {
    for (const auto &processor : m_processorPtrMap) {
        delete processor.second;
    }
}

void Core::run() {
    auto removeTempFunc = []() {
        FileSystem::removeDirectory(FileSystem::getTempPath());
    };
    if (std::atexit(removeTempFunc) != 0) {
        m_logger->warn("Failed to register exit function");
    }

    conditionalProcess();
}

void Core::conditionalProcess() {
    if (m_processorPtrMap.contains(ProcessorBase::FaceSwapper)) {
        processSourceFaces();
    }
    if (m_config->m_faceSelectorMode == FaceSelector::SelectorMode::Reference
        && m_faceStore->getFaces(std::string(m_referenceFacesStr)).empty()) {
        processReferenceFaces();
    }

    // clear ./temp
    FileSystem::removeDirectory(FileSystem::getTempPath());

    m_logger->info("[Core] Filtering target images...");
    std::unordered_set<std::string> targetImagePaths = FileSystem::filterImagePaths(m_config->m_targetPaths);

    if (!targetImagePaths.empty()) {
        conditionalProcessImages(targetImagePaths);
    }

    if (targetImagePaths.size() == m_config->m_targetPaths.size()) {
        return;
    }
    targetImagePaths.clear();

    m_logger->info("[Core] Filtering target videos...");
    std::unordered_set<std::string> targetVideoPaths = FfmpegRunner::filterVideoPaths(m_config->m_targetPaths);
    if (!targetVideoPaths.empty()) {
        conditionalProcessVideos(targetVideoPaths);
    }

    FileSystem::removeDirectory(FileSystem::getTempPath());
}

void Core::conditionalProcessImages(const std::unordered_set<std::string> &targetImagePaths) {
    if (targetImagePaths.empty()) {
        return;
    }

    std::vector<std::string> targetImagePathsVector(targetImagePaths.begin(), targetImagePaths.end());
    std::vector<std::string> normedOutputPaths;

    std::string tempPath = FileSystem::getTempPath();
    std::vector<std::string> tempTargetImagePaths;
    for (const auto &targetImagePath : targetImagePaths) {
        auto tempImagePath = tempPath + "/images/" + FileSystem::getFileName(targetImagePath);
        tempTargetImagePaths.emplace_back(FileSystem::absolutePath(tempImagePath));
    }

    m_logger->info("[Core] Coping images to temp: " + tempPath);
    if (!FileSystem::copyImages(targetImagePathsVector, tempTargetImagePaths, m_config->m_outputImageResolution)) {
        m_logger->error("[Core::conditionalProcess] Copy target images to temp path failed.");
        return;
    }

    std::string outputPath = FileSystem::absolutePath(m_config->m_outputPath);
    if (!FileSystem::directoryExists(outputPath)) {
        m_logger->info("[Core] Create output directory: " + outputPath);
        FileSystem::createDirectory(outputPath);
    }

    normedOutputPaths = FileSystem::normalizeOutputPaths(tempTargetImagePaths, outputPath);

    processImages(tempTargetImagePaths, tempTargetImagePaths);

    m_logger->info("[Core] Finalizing images...");
    if (!FileSystem::finalizeImages(tempTargetImagePaths, tempTargetImagePaths, m_config->m_outputImageResolution, m_config->m_outputImageQuality)) {
        m_logger->warn("[Core] Some images skipped finalization!");
    }

    m_logger->info("[Core] Moving processed images to output path...");
    FileSystem::moveFiles(tempTargetImagePaths, normedOutputPaths);

    m_logger->info(std::format("[Core] All images processed successfully. Output path: {}", FileSystem::absolutePath(m_config->m_outputPath)));
}

void Core::conditionalProcessVideos(const std::unordered_set<std::string> &videoPaths) {
    if (videoPaths.empty()) {
        return;
    }

    std::vector<std::string> targetVideoPaths(videoPaths.begin(), videoPaths.end());
    std::vector<std::string> tempTargetVideoPaths;
    for (const auto &targetVideoPath : targetVideoPaths) {
        std::string tempTargetVideoDir = FileSystem::getTempPath() + "/videos/" + FileSystem::getBaseName(targetVideoPath);
        while (FileSystem::directoryExists(tempTargetVideoDir)) {
            tempTargetVideoDir += "-" + FileSystem::generateRandomString(8);
        }
        std::string tempTargetVideoPath = tempTargetVideoDir + "/" + FileSystem::getFileName(targetVideoPath);
        tempTargetVideoPaths.emplace_back(FileSystem::absolutePath(tempTargetVideoPath));
    }

    m_logger->info("[Core] Coping videos to temp: " + FileSystem::absolutePath(FileSystem::getTempPath() + "/videos"));
    FileSystem::copyFiles(targetVideoPaths, tempTargetVideoPaths);

    std::string outputPath = FileSystem::absolutePath(m_config->m_outputPath);
    if (!FileSystem::directoryExists(outputPath)) {
        m_logger->info("[Core] Create output directory: " + outputPath);
        FileSystem::createDirectory(outputPath);
    }
    std::vector<std::string> normedOutputPaths = FileSystem::normalizeOutputPaths(tempTargetVideoPaths, m_config->m_outputPath);

    if (m_config->m_videoSegmentDuration > 0) {
        processVideosBySegments(tempTargetVideoPaths, normedOutputPaths, m_config->m_videoSegmentDuration);
    } else {
        processVideos(tempTargetVideoPaths, normedOutputPaths);
    }
}

ProcessorBase *Core::createProcessor(const ProcessorBase::ProcessorType &processorType) const {
    if (processorType == ProcessorBase::FaceSwapper) {
        m_logger->info("[Core] Initializing FaceSwapper");
        FaceSwapperBase *faceSwapper = FaceSwapperHelper::createFaceSwapper(m_config->m_faceSwapperModel, m_faceMaskers, m_env);
        faceSwapper->setMaskTypes(m_config->m_faceMaskTypeSet);
        return faceSwapper;
    }
    if (processorType == ProcessorBase::FaceEnhancer) {
        m_logger->info("[Core] Initializing FaceEnhancer");
        FaceEnhancerBase *faceEnhancer = FaceEnhancerHelper::createFaceEnhancer(m_config->m_faceEnhancerModel, m_faceMaskers, m_env);
        faceEnhancer->setMaskTypes(m_config->m_faceMaskTypeSet);
        return faceEnhancer;
    }
    if (processorType == ProcessorBase::ExpressionRestorer) {
        m_logger->info("[Core] Initializing ExpressionRestorer");
        std::string featureExPath = ModelManager::getInstance()->getModelPath(ModelManager::Model::Feature_extractor);
        std::string motionExPath = ModelManager::getInstance()->getModelPath(ModelManager::Model::Motion_extractor);
        std::string generatorPath = ModelManager::getInstance()->getModelPath(ModelManager::Model::Generator);
        auto *expressionRestorer = new ExpressionRestorer(m_env, m_faceMaskers, featureExPath, motionExPath, generatorPath);
        expressionRestorer->setMaskTypes(m_config->m_faceMaskTypeSet);
        return expressionRestorer;
    }
    if (processorType == ProcessorBase::FrameEnhancer) {
        m_logger->info("[Core] Initializing FrameEnhancer");
        FrameEnhancerBase *frameEnhancer = FrameEnhancerHelper::CreateFrameEnhancer(m_config->m_frameEnhancerModel, m_env);
        frameEnhancer->setBlend(m_config->m_frameEnhancerBlend);
        return frameEnhancer;
    }
    return nullptr;
}

void Core::processReferenceFaces() {
    if (m_config->m_referenceFacePath.empty()) {
        m_logger->error("[Core] Reference face path is empty.");
        std::exit(1);
    }

    if (!FileSystem::isImage(m_config->m_referenceFacePath)) {
        m_logger->error("[Core] Reference face path is not a valid image file.");
        std::exit(1);
    }

    auto referenceFrame = Vision::readStaticImage(m_config->m_referenceFacePath);
    auto referenceFace = m_faceAnalyser->getOneFace(referenceFrame, m_config->m_referenceFacePosition);

    if (referenceFace.isEmpty()) {
        m_logger->error("[Core] No face found in the reference image.");
        std::exit(1);
    } else {
        m_faceStore->appendFaces(std::string{m_referenceFacesStr}, {referenceFace});
    }
}

void Core::processSourceFaces() {
    std::unordered_set<std::string> sourcePaths = m_config->m_sourcePaths;
    if (sourcePaths.empty()) {
        m_logger->warn("[Core] No image found in the source paths.");
    }

    auto sourceFrames = Vision::multiReadStaticImages(sourcePaths);
    auto sourceAverageFace = m_faceAnalyser->getAverageFace(sourceFrames);
    if (sourceAverageFace.isEmpty()) {
        m_logger->error("[Core] No face found in the source images.");
        std::exit(1);
    } else {
        m_faceStore->appendFaces(std::string{m_sourceAverageFace}, {sourceAverageFace});
    }
}

bool Core::processImage(const std::string &targetImagePath,
                        const std::string &originalTargetImagePath,
                        const std::string &outputImagePath,
                        ProcessorBase *processor) {
    cv::Mat targetFrame = Vision::readStaticImage(targetImagePath);
    ProcessorBase::InputData inputData;
    std::unordered_set<ProcessorBase::InputDataType> inputTypes = processor->getInputDataTypes();

    if (inputTypes.contains(ProcessorBase::SourceFaces)) {
        std::vector<Face> sourceFaces = m_faceStore->getFaces(std::string{m_sourceAverageFace});
        inputData.m_sourceFaces = new std::vector<Face>(sourceFaces);
    }
    if (inputTypes.contains(ProcessorBase::TargetFaces)) {
        std::vector<Face> targetFaces;
        if (m_config->m_faceSelectorMode == FaceSelector::SelectorMode::Many) {
            targetFaces = m_faceAnalyser->getManyFaces(targetFrame);
        } else if (m_config->m_faceSelectorMode == FaceSelector::SelectorMode::One) {
            Face oneFace = m_faceAnalyser->getOneFace(targetFrame, m_config->m_referenceFacePosition);
            if (!oneFace.isEmpty()) {
                targetFaces.emplace_back(oneFace);
            }
        } else if (m_config->m_faceSelectorMode == FaceSelector::SelectorMode::Reference) {
            std::vector<Face> referenceFaces = m_faceStore->getFaces(std::string{m_referenceFacesStr});
            std::vector<Face> similarFaces = m_faceAnalyser->findSimilarFaces(referenceFaces, targetFrame, m_config->m_referenceFaceDistance);
            if (!similarFaces.empty()) {
                targetFaces.insert(targetFaces.end(), similarFaces.begin(), similarFaces.end());
            } else {
                if (!Vision::writeImage(targetFrame, outputImagePath)) {
                    m_logger->error("[Core::processImage] Write image failed: " + outputImagePath);
                    return false;
                }
                return true;
            }
        }
        inputData.m_targetFaces = new std::vector<Face>(targetFaces);
    }
    if (inputTypes.contains(ProcessorBase::TargetFrame)) {
        inputData.m_targetFrame = new cv::Mat(targetFrame);
    }
    if (inputTypes.contains(ProcessorBase::OriginalTargetFrame)) {
        inputData.m_originalTargetFrame = new cv::Mat(Vision::readStaticImage(originalTargetImagePath));
    }

    cv::Mat resultFrame = processor->processFrame(&inputData);
    if (!resultFrame.empty()) {
        if (!Vision::writeImage(resultFrame, outputImagePath)) {
            m_logger->error("[Core::processImage] Write image failed: " + outputImagePath);
            return false;
        }
        return true;
    } else {
        m_logger->error("[Core::processImage] Process frame failed(resultFrame is empty): " + outputImagePath);
        return false;
    }
}

void Core::processImages(const std::vector<std::string> &targetImagePaths,
                         const std::vector<std::string> &outputImagePaths) {
    if (targetImagePaths.empty()) {
        m_logger->error("[Core::processImages] inputImagePaths is empty.");
        return;
    }
    if (outputImagePaths.empty()) {
        m_logger->error("[Core::ProcessImages] outputImagePaths is empty.");
        return;
    }
    if (targetImagePaths.size() != outputImagePaths.size()) {
        m_logger->error("[Core::ProcessImages] inputImagePaths and outputImagePaths size mismatch.");
        return;
    }

    std::vector<std::string> originalTargetImagePaths;

    if (std::find(m_config->m_frameProcessors.cbegin(), m_config->m_frameProcessors.cend(), ProcessorBase::ExpressionRestorer) != m_config->m_frameProcessors.cend()) {
        for (const auto &targetImagePath : targetImagePaths) {
            std::string tempTargetImagePath = std::filesystem::path(targetImagePath).parent_path().string() + "/" + FileSystem::getBaseName(targetImagePath) + "_original" + FileSystem::getExtension(targetImagePath);
            originalTargetImagePaths.emplace_back(tempTargetImagePath);
        }
        FileSystem::copyFiles(targetImagePaths, originalTargetImagePaths);
    } else {
        originalTargetImagePaths = targetImagePaths;
    }

    dp::thread_pool pool(m_config->m_executionThreadCount);
    for (const auto &type : m_config->m_frameProcessors) {
        ProcessorBase *processor = nullptr;
        if (m_processorPtrMap.contains(type)) {
            processor = m_processorPtrMap[type];
            if (processor == nullptr) {
                m_processorPtrMap[type] = createProcessor(type);
                processor = m_processorPtrMap[type];
            }
        } else {
            processor = createProcessor(type);
            m_processorPtrMap[type] = processor;
        }
        if (processor == nullptr) {
            m_logger->error("[Core::processImages] processor is nullPrt.");
            throw std::runtime_error("[Core::processImages] processor is nullPrt.");
        }

        std::vector<std::future<bool>> futures;
        for (size_t i = 0; i < targetImagePaths.size(); ++i) {
            futures.emplace_back(pool.enqueue([this, &originalTargetImagePaths, &targetImagePaths, &outputImagePaths, i, processor]() {
                return processImage(targetImagePaths[i], originalTargetImagePaths[i], outputImagePaths[i], processor);
            }));
        }

        std::string processorName = processor->getProcessorName();
        const size_t numTargetPaths = targetImagePaths.size();
        show_console_cursor(false);
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
                m_logger->error(std::format("[{}] Failed to process image: {}", processorName, targetImagePaths[i]));
                ++i;
                continue;
            }

            auto writeIsSuccess = futures[i].get();
            if (!writeIsSuccess) {
                isAllWriteSuccess = false;
                m_logger->error(std::format("[{}] Failed to write image: {}", processorName, outputImagePaths[i]));
            }

            bar.setPostfixText(std::format("{}/{}", (i + 1), numTargetPaths));
            int progress = static_cast<int>(std::floor(((float)(i + 1) * 100.0f) / (float)numTargetPaths));
            bar.setProgress(progress);

            ++i;
            if (i >= outputImagePaths.size()) {
                break;
            }
        }
        show_console_cursor(true);
        if (!isAllWriteSuccess) {
            m_logger->error(std::format("[{}] Some images failed to process or write.", processorName));
        }

        if (m_config->m_processorMemoryStrategy == Config::ProcessorMemoryStrategy::Strict) {
            delete m_processorPtrMap[type];
            m_processorPtrMap[type] = nullptr;
        }
    }

    if (std::find(m_config->m_frameProcessors.cbegin(), m_config->m_frameProcessors.cend(), ProcessorBase::ExpressionRestorer) != m_config->m_frameProcessors.cend()) {
        pool.enqueue(&FileSystem::removeFiles, originalTargetImagePaths);
    }
}

bool Core::processVideo(const std::string &videoPath, const std::string &outputVideoPath, const bool &skipAudio) {
    std::filesystem::path pathVideo(videoPath);

    std::string audiosDir = FileSystem::absolutePath(pathVideo.parent_path().string() + "/audios");
    if (!skipAudio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(m_config->m_outputAudioEncoder);
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    m_logger->info(std::format("[Core] Extract Frames for {}", videoPath));
    std::string pattern = "frame_%06d." + m_config->m_tempFrameFormat;
    std::string videoFramesOutputDir = FileSystem::absolutePath(pathVideo.parent_path().string() + "/" + Ffc::FileSystem::getBaseName(videoPath));
    std::string outputPattern = videoFramesOutputDir + "/" + pattern;
    Ffc::FfmpegRunner::extractFrames(videoPath, outputPattern);
    std::unordered_set<std::string> framePaths = FileSystem::listFilesInDirectory(videoFramesOutputDir);
    framePaths = Ffc::FileSystem::filterImagePaths(framePaths);
    std::vector<std::string> framePathsVec(framePaths.begin(), framePaths.end());

    processImages(framePathsVec, framePathsVec);

    Ffc::FfmpegRunner::VideoPrams videoPrams(videoPath);
    videoPrams.quality = m_config->m_outputVideoQuality;
    videoPrams.preset = m_config->m_outputVideoPreset;
    videoPrams.videoCodec = m_config->m_outputVideoEncoder;
    if (!framePathsVec.empty()) {
        cv::Mat firstFrame = Vision::readStaticImage(framePathsVec[0]);
        videoPrams.width = firstFrame.cols;
        videoPrams.height = firstFrame.rows;
    }

    std::string inputImagePattern = videoFramesOutputDir + "/" + pattern;
    std::string outputVideo_NA_Path = pathVideo.parent_path().string() + "/" + Ffc::FileSystem::getBaseName(videoPath) + "_processed_NA" + Ffc::FileSystem::getExtension(videoPath);
    Ffc::Logger::getInstance()->info("[Core] Images to video : " + FileSystem::absolutePath(outputVideo_NA_Path));
    if (!Ffc::FfmpegRunner::imagesToVideo(inputImagePattern, outputVideo_NA_Path, videoPrams)) {
        Ffc::Logger::getInstance()->error("[Core] images to video failed!");
        FileSystem::removeDirectory(videoFramesOutputDir);
        FileSystem::removeFile(outputVideo_NA_Path);
        return false;
    }

    if (!skipAudio) {
        std::unordered_set<std::string> audioPaths = Ffc::FileSystem::listFilesInDirectory(audiosDir);
        audioPaths = Ffc::FfmpegRunner::filterAudioPaths(audioPaths);
        std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

        Ffc::Logger::getInstance()->info("[Core] Add audios to video : " + FileSystem::absolutePath(outputVideoPath));
        if (!Ffc::FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, outputVideoPath)) {
            Ffc::Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
        }
    } else {
        try {
            Ffc::FileSystem::moveFile(outputVideo_NA_Path, outputVideoPath);
        } catch (const std::exception &e) {
            Ffc::Logger::getInstance()->error("[Core] Move video failed! Error:" + std::string(e.what()));
            return false;
        }
    }

    Ffc::FileSystem::removeDirectory(videoFramesOutputDir);
    Ffc::FileSystem::removeDirectory(audiosDir);
    Ffc::FileSystem::removeFile(outputVideo_NA_Path);
    return true;
}

bool Core::processVideoBySegments(const std::string &videoPath, const std::string &outputVideoPath, const unsigned int &duration) {
    std::filesystem::path pathVideo(videoPath);

    std::string audiosDir = FileSystem::absolutePath(pathVideo.parent_path().string() + "/audios");
    if (!m_config->m_skipAudio) {
        FfmpegRunner::Audio_Codec audioCodec = FfmpegRunner::getAudioCodec(m_config->m_outputAudioEncoder);
        if (audioCodec == FfmpegRunner::Audio_Codec::Codec_UNKNOWN) {
            m_logger->warn("[Core] Unsupported audio codec. Use Default: aac");
            audioCodec = FfmpegRunner::Audio_Codec::Codec_AAC;
        }
        m_logger->info(std::format("[Core] Extract Audios for {}", videoPath));
        FfmpegRunner::extractAudios(videoPath, audiosDir, audioCodec);
    }

    std::string videoSegmentsDir = pathVideo.parent_path().string() + "/videoSegments";
    std::string videoSegmentPattern = "segment_%03d" + FileSystem::getExtension(videoPath);
    Ffc::Logger::getInstance()->info(std::format("[Core] Divide the video into segments of {} seconds each....", duration));
    if (!Ffc::FfmpegRunner::cutVideoIntoSegments(videoPath, videoSegmentsDir, duration, videoSegmentPattern)) {
        Ffc::Logger::getInstance()->error("The attempt to cut the video into segments was failed!");
        FileSystem::removeDirectory(audiosDir);
        return false;
    }

    std::unordered_set<std::string> videoSegmentsPaths = Ffc::FileSystem::listFilesInDirectory(videoSegmentsDir);
    videoSegmentsPaths = Ffc::FfmpegRunner::filterVideoPaths(videoSegmentsPaths);

    std::vector<std::string> processedVideoSegmentsPaths;
    std::string processedVideoSegmentsDir = pathVideo.parent_path().string() + "/videoSegments_processed";
    std::vector<std::string> videoSegmentsPathsVec(videoSegmentsPaths.begin(), videoSegmentsPaths.end());
    std::sort(videoSegmentsPathsVec.begin(), videoSegmentsPathsVec.end(), [](const std::string &a, const std::string &b) {
        return a < b;
    });
    size_t segmentIndex = 0;
    for (const auto &videoSegmentPath : videoSegmentsPathsVec) {
        m_logger->info(std::format("[Core] Processing video segment {}/{}", segmentIndex + 1, videoSegmentsPaths.size()));
        segmentIndex++;

        std::string outputVideoSegmentPath = FileSystem::absolutePath(processedVideoSegmentsDir + "/" + Ffc::FileSystem::getFileName(videoSegmentPath));
        if (!processVideo(videoSegmentPath, outputVideoSegmentPath, true)) {
            m_logger->error(std::format("[Core] Failed to process video segment: {}", videoSegmentPath));
            return false;
        }
        processedVideoSegmentsPaths.emplace_back(outputVideoSegmentPath);

        Ffc::FileSystem::removeFile(videoSegmentPath);
    }
    Ffc::FileSystem::removeDirectory(videoSegmentsDir);

    Ffc::FfmpegRunner::VideoPrams videoPrams(processedVideoSegmentsPaths[0]);
    videoPrams.quality = m_config->m_outputVideoQuality;
    videoPrams.preset = m_config->m_outputVideoPreset;
    videoPrams.videoCodec = m_config->m_outputVideoEncoder;

    std::string outputVideo_NA_Path = pathVideo.parent_path().string() + "/" + Ffc::FileSystem::getBaseName(videoPath) + "_processed_NA" + Ffc::FileSystem::getExtension(videoPath);
    Ffc::Logger::getInstance()->info("[Core] concat video segments...");
    if (!Ffc::FfmpegRunner::concatVideoSegments(processedVideoSegmentsPaths, outputVideo_NA_Path, videoPrams)) {
        Ffc::Logger::getInstance()->error("[Core] Failed concat video segments for : " + videoPath);
        FileSystem::removeDirectory(processedVideoSegmentsDir);
        return false;
    }

    std::unordered_set<std::string> audioPaths = Ffc::FileSystem::listFilesInDirectory(audiosDir);
    audioPaths = Ffc::FfmpegRunner::filterAudioPaths(audioPaths);
    std::vector<std::string> audioPathsVec(audioPaths.begin(), audioPaths.end());

    Ffc::Logger::getInstance()->info("[Core] Add audios to video...");
    if (!Ffc::FfmpegRunner::addAudiosToVideo(outputVideo_NA_Path, audioPathsVec, outputVideoPath)) {
        Ffc::Logger::getInstance()->warn("[Core] Add audios to Video failed. The output video will be without audio.");
    }

    Ffc::FileSystem::removeDirectory(audiosDir);
    Ffc::FileSystem::removeFile(outputVideo_NA_Path);
    Ffc::FileSystem::removeDirectory(processedVideoSegmentsDir);
    return true;
}

void Core::processVideos(const std::vector<std::string> &videoPaths, const std::vector<std::string> &outputVideoPaths) {
    if (videoPaths.empty()) {
        m_logger->error("[Core::ProcessVideos] videoPaths is empty.");
        return;
    }
    if (outputVideoPaths.empty()) {
        m_logger->error("[Core::ProcessVideos] outputVideoPaths is empty.");
        return;
    }
    if (videoPaths.size() != outputVideoPaths.size()) {
        m_logger->error("[Core::ProcessVideos] videoPaths and outputVideoPaths size mismatch.");
        return;
    }

    for (size_t i = 0; i < videoPaths.size(); ++i) {
        if (processVideo(videoPaths[i], outputVideoPaths[i])) {
            m_logger->info(std::format("[Core] Video processed successfully. Output path : {}", outputVideoPaths[i]));
        } else {
            m_logger->error(std::format("[Core] Video {} processed failed.", videoPaths[i]));
        }
    }
}

void Core::processVideosBySegments(const std::vector<std::string> &videoPaths, const std::vector<std::string> &outputVideoPaths, const unsigned int &secondDuration) {
    if (videoPaths.empty()) {
        m_logger->error("[Core::ProcessVideosBySegments] videoPaths is empty.");
        return;
    }
    if (outputVideoPaths.empty()) {
        m_logger->error("[Core::ProcessVideosBySegments] outputVideoPaths is empty.");
        return;
    }
    if (videoPaths.size() != outputVideoPaths.size()) {
        m_logger->error("[Core::ProcessVideosBySegments] videoPaths and outputVideoPaths size mismatch.");
        return;
    }

    for (size_t i = 0; i < videoPaths.size(); ++i) {
        if (processVideoBySegments(videoPaths[i], outputVideoPaths[i], secondDuration)) {
            m_logger->info(std::format("[Core] Video processed successfully. Output path: {} ", outputVideoPaths[i]));
        } else {
            m_logger->error(std::format("[Core] Video {} processed failed.", videoPaths[i]));
        }
    }
}
} // namespace Ffc