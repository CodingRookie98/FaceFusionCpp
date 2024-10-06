/**
 ******************************************************************************
 * @file           : config.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-17
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_CONFIG_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_CONFIG_H_

#include <string>
#include <vector>
#include <unordered_set>
#include <shared_mutex>
#include <SimpleIni.h>
#include <opencv2/opencv.hpp>
#include "logger.h"
#include "processor_base.h"
#include "face_detectors.h"
#include "model_manager.h"
#include "face_swapper_helper.h"
#include "face_enhancer_helper.h"
#include "face_selector.h"
#include "face_maskers.h"
#include "face_masker_region.h"
#include "inference_session.h"
#include "face_landmarkers.h"
#include "frame_enhancer_helper.h"

namespace Ffc {
class Config {
public:
    explicit Config(const std::string &configPath = "./faceFusionCpp.ini");
    ~Config() = default;
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&) = delete;
    Config &operator=(Config &&) = delete;

    static std::shared_ptr<Config> getInstance(const std::string &configPath = "./faceFusionCpp.ini");

    // general
    std::unordered_set<std::string> m_sourcePaths;
    std::unordered_set<std::string> m_targetPaths;
    std::string m_referenceFacePath;
    std::string m_outputPath;

    // misc
    bool m_forceDownload;
    bool m_skipDownload;
    Logger::LogLevel m_logLevel;

    // execution
    int m_executionDeviceId;
    std::unordered_set<InferenceSession::ExecutionProvider> m_executionProviders;
    int m_executionThreadCount;

    // tensort
    bool m_enableTensorrtCache;
    bool m_enableTensorrtEmbedEngine;
    size_t m_trtMaxWorkspaceSize;

    // memory
    enum ProcessorMemoryStrategy {
        Strict,
        Tolerant,
    };
    ProcessorMemoryStrategy m_processorMemoryStrategy;

    // face analyser
    float m_faceDetectorScore;
    FaceLandmarkers::Landmarker68Model  m_faceLandmarkerModel;
    float m_faceLandmarkerScore;
    FaceDetectors::FaceDetectorType m_faceDetectorModel;
    cv::Size m_faceDetectorSize;

    // face selector
    FaceSelector::SelectorMode m_faceSelectorMode;
    FaceSelector::FaceSelectorOrder m_faceSelectorOrder;
    FaceSelector::Gender m_faceSelectorGender;
    FaceSelector::Race m_faceSelectorRace;
    unsigned int m_faceSelectorAgeStart, m_faceSelectorAgeEnd;
    unsigned int m_referenceFacePosition;
    float m_referenceFaceDistance;
    unsigned int m_referenceFrameNumber;

    // face masker
    std::unordered_set<FaceMaskers::Type> m_faceMaskTypeSet;
    float m_faceMaskBlur;
    std::array<int, 4> m_faceMaskPadding;
    std::unordered_set<FaceMaskerRegion::Region> m_faceMaskRegionsSet;

    // output creation
    int m_outputImageQuality;
    cv::Size m_outputImageResolution;

    // video
    unsigned int m_videoSegmentDuration;
    std::string m_outputVideoEncoder;
    std::string m_outputVideoPreset;
    unsigned int m_outputVideoQuality;
    std::string m_outputAudioEncoder;
    bool m_skipAudio;
    std::string m_tempFrameFormat;

    // Frame Processors
    std::vector<ProcessorBase::ProcessorType> m_frameProcessors;
    FaceSwapperHelper::FaceSwapperModel m_faceSwapperModel;
    FaceEnhancerHelper::Model m_faceEnhancerModel;
    int m_faceEnhancerBlend;
    float m_expressionRestorerFactor;
    FrameEnhancerHelper::Model m_frameEnhancerModel;
    int m_frameEnhancerBlend;

private:
    CSimpleIniA m_ini;
    std::shared_mutex m_sharedMutex;
    std::string m_configPath;
    std::shared_ptr<Logger> m_logger = Logger::getInstance();
    void loadConfig();
    static std::array<int, 4> normalizePadding(const std::vector<int> &padding);
    static std::vector<int> parseStringToVector(const std::string &input);

    void general();
    void misc();
    void execution();
    void tensort();
    void memory();
    void faceAnalyser();
    void faceSelector();
    void faceMasker();
    void image();
    void video();
    void frameProcessors();
    static void tolower(std::string &str);
};
} // namespace Ffc

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_CONFIG_H_
