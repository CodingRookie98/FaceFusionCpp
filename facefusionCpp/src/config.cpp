/**
 ******************************************************************************
 * @file           : config.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-17
 ******************************************************************************
 */

#include "config.h"
#include <algorithm>
#include <regex>
#include "file_system.h"
#include "vision.h"

namespace Ffc {
Config::Config(const std::string &configPath) {
    if (!configPath.empty() && FileSystem::fileExists(configPath)) {
        m_configPath = configPath;
        loadConfig();
    } else {
        m_logger->error(std::format("Config file not found: {}", FileSystem::absolutePath(configPath)));
        std::exit(1);
    }
}

void Config::loadConfig() {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);

    m_ini.SetUnicode();
    SI_Error rc = m_ini.LoadFile(m_configPath.c_str());
    if (rc < 0) {
        m_logger->error("Failed to load config file");
        throw std::runtime_error("Failed to load config file");
    };

    general();
    misc();
    execution();
    tensort();
    memory();
    faceAnalyser();
    faceSelector();
    faceMasker();
    image();
    video();
    frameProcessors();
}

std::array<int, 4> Config::normalizePadding(const std::vector<int> &padding) {
    if (padding.size() == 1) {
        return {padding[0], padding[0], padding[0], padding[0]};
    } else if (padding.size() == 2) {
        return {padding[0], padding[1], padding[0], padding[1]};
    } else if (padding.size() == 3) {
        return {padding[0], padding[1], padding[2], padding[1]};
    } else if (padding.size() == 4) {
        return {padding[0], padding[1], padding[2], padding[3]};
    } else {
        throw std::invalid_argument("Invalid padding length");
    }
}

std::vector<int> Config::parseStringToVector(const std::string &input) {
    std::vector<int> values;
    std::stringstream ss(input);
    int value;
    while (ss >> value) {
        values.push_back(value);
    }
    if (ss.fail() && !ss.eof()) {
        throw std::invalid_argument("Invalid input format");
    }
    return values;
}

void Config::frameProcessors() {
    // frame_processors
    std::string value = m_ini.GetValue("frame_processors", "frame_processors", "face_swapper");
    tolower(value);
    if (!value.empty()) {
        std::vector<std::string> result;

        // 使用正则表达式分割字符串，匹配任意多个空格
        std::regex re("\\s+");
        std::sregex_token_iterator iter(value.begin(), value.end(), re, -1);
        std::sregex_token_iterator end;

        // 将分割的字符串存入vector
        for (; iter != end; ++iter) {
            if (!iter->str().empty()) {
                result.push_back(*iter);
            }
        }
        bool flag = false;
        for (const auto &item : result) {
            if (item == "face_swapper") {
                m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::FaceSwapper);
                flag = true;
            }
            if (item == "face_enhancer") {
                m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::FaceEnhancer);
                flag = true;
            }
            if (item == "expression_restorer") {
                m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::ExpressionRestorer);
                flag = true;
            }
            if (item == "frame_enhancer") {
                m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::FrameEnhancer);
                flag = true;
            }
        }
        if (!flag) {
            m_logger->warn("[Config] The user-specified frame processors are not supported; using the default: face_swapper.");
            m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::FaceSwapper);
        }
    } else {
        m_logger->warn("[Config] No frame processors specified, using default: face_swapper");
        m_frameProcessors.emplace_back(ProcessorBase::ProcessorType::FaceSwapper);
    }

    value = m_ini.GetValue("frame_processors", "face_enhancer_model", "gfpgan_1.4");
    tolower(value);
    if (!value.empty()) {
        if (value == "codeformer") {
            m_faceEnhancerModel = FaceEnhancerHelper::Model::_CodeFormer;
        } else if (value == "gfpgan_1.2") {
            m_faceEnhancerModel = FaceEnhancerHelper::Model::Gfpgan_12;
        } else if (value == "gfpgan_1.3") {
            m_faceEnhancerModel = FaceEnhancerHelper::Model::Gfpgan_13;
        } else if (value == "gfpgan_1.4") {
            m_faceEnhancerModel = FaceEnhancerHelper::Model::Gfpgan_14;
        } else {
            std::cerr << "Invalid face enhancer model: " << value << " Use Default: gfpgan_1.4" << std::endl;
            m_faceEnhancerModel = FaceEnhancerHelper::Model::Gfpgan_14;
        }
    } else {
        m_faceEnhancerModel = FaceEnhancerHelper::Model::Gfpgan_14;
    }

    value = m_ini.GetValue("frame_processors", "face_enhancer_blend", "80");
    if (!value.empty()) {
        m_faceEnhancerBlend = std::stoi(value);
        if (m_faceEnhancerBlend < 0) {
            m_faceEnhancerBlend = 0;
        } else if (m_faceEnhancerBlend > 100) {
            m_faceEnhancerBlend = 100;
        }
    } else {
        m_faceEnhancerBlend = 80;
    }

    value = m_ini.GetValue("frame_processors", "face_swapper_model", "inswapper_128_fp16");
    tolower(value);
    if (!value.empty()) {
        if (value == "inswapper_128_fp16") {
            m_faceSwapperModel = FaceSwapperHelper::FaceSwapperModel::Inswapper_128_fp16;
        } else if (value == "inswapper_128") {
            m_faceSwapperModel = FaceSwapperHelper::FaceSwapperModel::Inswapper_128;
        } else {
            m_logger->warn(std::format("[Config] Invalid face swapper model: {}, Use Default: inswapper_128_fp16", value));
            m_faceSwapperModel = FaceSwapperHelper::FaceSwapperModel::Inswapper_128_fp16;
        }
    } else {
        m_faceSwapperModel = FaceSwapperHelper::FaceSwapperModel::Inswapper_128_fp16;
    }

    value = m_ini.GetValue("frame_processors", "expression_restorer_factor", "80");
    if (!value.empty()) {
        m_expressionRestorerFactor = std::stof(value);
        if (m_expressionRestorerFactor < 0) {
            m_expressionRestorerFactor = 0;
        } else if (m_expressionRestorerFactor > 100) {
            m_expressionRestorerFactor = 100;
        }
    } else {
        m_expressionRestorerFactor = 80;
    }
    m_expressionRestorerFactor = (m_expressionRestorerFactor / 100.0f) * 1.2f;

    value = m_ini.GetValue("frame_processors", "frame_enhancer_model", "real_hatgan_x4");
    tolower(value);
    if (!value.empty()) {
        if (value == "real_esrgan_x2") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x2;
        } else if (value == "real_esrgan_x2_fp16") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x2_fp16;
        } else if (value == "real_esrgan_x4") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x4;
        } else if (value == "real_esrgan_x4_fp16") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x4_fp16;
        } else if (value == "real_esrgan_x8") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x8;
        } else if (value == "real_esrgan_x8_fp16") {
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_esrgan_x8_fp16;
        }else if(value == "real_hatgan_x4"){
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_hatgan_x4;
        } else {
            m_logger->warn(std::format("[Config] Invalid frame enhancer: {}, Use Default: real_hatgan_x4", value));
            m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_hatgan_x4;
        }
    } else {
        m_frameEnhancerModel = FrameEnhancerHelper::Model::Real_hatgan_x4;
    }

    value = m_ini.GetValue("frame_processors", "frame_enhancer_blend", "80");
    if (!value.empty()) {
        m_frameEnhancerBlend = std::stoi(value);
        if (m_frameEnhancerBlend < 0) {
            m_frameEnhancerBlend = 0;
        } else if (m_frameEnhancerBlend > 100) {
            m_frameEnhancerBlend = 100;
        }
    } else {
        m_frameEnhancerBlend = 80;
    }
}

void Config::image() {
    // output_creation
    std::string value = m_ini.GetValue("image", "output_image_quality", "100");
    if (!value.empty()) {
        m_outputImageQuality = std::stoi(value);
        if (m_outputImageQuality < 0) {
            m_outputImageQuality = 0;
        } else if (m_outputImageQuality > 100) {
            m_outputImageQuality = 100;
        }
    } else {
        m_outputImageQuality = 100;
    }
    value = m_ini.GetValue("image", "output_image_resolution", "");
    if (!value.empty()) {
        m_outputImageResolution = Vision::unpackResolution(value);
    } else {
        m_outputImageResolution = cv::Size(0, 0);
    }
}

void Config::faceMasker() {
    // face_mask
    std::string value = m_ini.GetValue("face_mask", "face_mask_types", "box");
    if (!value.empty()) {
        if (value.find("box") != std::string::npos) {
            m_faceMaskTypeSet.insert(FaceMaskers::Type::Box);
        }
        if (value.find("occlusion") != std::string::npos) {
            m_faceMaskTypeSet.insert(FaceMaskers::Type::Occlusion);
        }
        if (value.find("region") != std::string::npos) {
            m_faceMaskTypeSet.insert(FaceMaskers::Type::Region);
        }
    } else {
        m_faceMaskTypeSet.insert(FaceMaskers::Type::Box);
    }

    value = m_ini.GetValue("face_mask", "face_mask_blur", "0.3");
    if (!value.empty()) {
        m_faceMaskBlur = std::stof(value);
        if (m_faceMaskBlur < 0) {
            m_faceMaskBlur = 0;
        } else if (m_faceMaskBlur > 1) {
            m_faceMaskBlur = 1;
        }
    } else {
        m_faceMaskBlur = 0.3f;
    }

    value = m_ini.GetValue("face_mask", "face_mask_padding", "0 0 0 0");
    if (!value.empty()) {
        m_faceMaskPadding = normalizePadding(parseStringToVector(value));
    } else {
        m_faceMaskPadding = {0, 0, 0, 0};
    }

    value = m_ini.GetValue("face_mask", "face_mask_region", "All");
    tolower(value);
    if (!value.empty()) {
        if (value == "all") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::All);
        } else if (value == "skin") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::Skin);
        } else if (value == "nose") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::Nose);
        } else if (value == "left-eyebrow") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::LeftEyebrow);
        } else if (value == "right-eyebrow") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::RightEyebrow);
        } else if (value == "mouth") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::Mouth);
        } else if (value == "right-eye") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::RightEye);
        } else if (value == "left-eye") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::LeftEye);
        } else if (value == "glasses") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::Glasses);
        } else if (value == "upper-lip") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::UpperLip);
        } else if (value == "lower-lip") {
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::LowerLip);
        } else {
            m_logger->warn("[Config] Invalid face mask region: " + value + " Use default: All");
            m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::All);
        }
    } else {
        m_faceMaskRegionsSet.insert(FaceMaskerRegion::Region::All);
    }
}
void Config::faceSelector() {
    // face_selector
    std::string value = m_ini.GetValue("face_selector", "face_selector_mode", "reference");
    if (!value.empty() && m_referenceFacePath.empty()) {
        if (value == "reference") {
            m_faceSelectorMode = FaceSelector::SelectorMode::Reference;
        } else if (value == "one") {
            m_faceSelectorMode = FaceSelector::SelectorMode::One;
        } else if (value == "many") {
            m_faceSelectorMode = FaceSelector::SelectorMode::Many;
        } else {
            m_logger->warn("[Config] Invalid face selector mode: " + value + " Use default: reference");
            m_faceSelectorMode = FaceSelector::SelectorMode::Many;
        }
    } else {
        if (!m_referenceFacePath.empty()) {
            m_faceSelectorMode = FaceSelector::SelectorMode::Reference;
        } else {
            m_faceSelectorMode = FaceSelector::SelectorMode::Many;
        }
    }

    value = m_ini.GetValue("face_selector", "face_selector_order", "left-right");
    if (!value.empty()) {
        if (value == "left-right") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Left_Right;
        } else if (value == "right-left") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Right_Left;
        } else if (value == "top-bottom") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Top_Bottom;
        } else if (value == "bottom-top") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Bottom_Top;
        } else if (value == "smAll-large") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Small_Large;
        } else if (value == "large-smAll") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Large_Small;
        } else if (value == "best-worst") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Best_Worst;
        } else if (value == "worst-best") {
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Worst_Best;
        } else {
            m_logger->warn("[Config] Invalid face selector order: " + value + " Use default: left-right");
            m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Left_Right;
        }
    } else {
        m_faceSelectorOrder = FaceSelector::FaceSelectorOrder::Left_Right;
    }

    value = m_ini.GetValue("face_selector", "face_selector_age_start", "0");
    if (!value.empty()) {
        m_faceSelectorAgeStart = std::stoi(value);
        if (m_faceSelectorAgeStart < 0) {
            m_faceSelectorAgeStart = 0;
        } else if (m_faceSelectorAgeStart > 100) {
            m_faceSelectorAgeStart = 70;
        }
    } else {
        m_faceSelectorAgeStart = 0;
    }
    value = m_ini.GetValue("face_selector", "face_selector_age_end", "100");
    if (!value.empty()) {
        m_faceSelectorAgeEnd = std::stoi(value);
        if (m_faceSelectorAgeEnd < 0 || m_faceSelectorAgeEnd > 100) {
            m_faceSelectorAgeEnd = 100;
        }
    } else {
        m_faceSelectorAgeEnd = 100;
    }

    value = m_ini.GetValue("face_selector", "face_selector_gender", "All");
    tolower(value);
    if (!value.empty()) {
        if (value == "male") {
            m_faceSelectorGender = FaceSelector::Gender::Male;
        } else if (value == "female") {
            m_faceSelectorGender = FaceSelector::Gender::Female;
        } else if (value == "all") {
            m_faceSelectorGender = FaceSelector::Gender::AllGender;
        } else {
            m_logger->warn("[Config] Invalid face selector gender: " + value + " Use default: All");
            m_faceSelectorGender = FaceSelector::Gender::AllGender;
        }
    } else {
        m_faceSelectorGender = FaceSelector::Gender::AllGender;
    }

    value = m_ini.GetValue("face_selector", "face_selector_race", "All");
    if (!value.empty()) {
        if (value == "white") {
            m_faceSelectorRace = FaceSelector::Race::White;
        } else if (value == "black") {
            m_faceSelectorRace = FaceSelector::Race::Black;
        } else if (value == "asian") {
            m_faceSelectorRace = FaceSelector::Race::Asian;
        } else if (value == "latino") {
            m_faceSelectorRace = FaceSelector::Race::Latino;
        } else if (value == "arabic") {
            m_faceSelectorRace = FaceSelector::Race::Arabic;
        } else if (value == "india") {
            m_faceSelectorRace = FaceSelector::Race::Indian;
        } else if (value == "All") {
            m_faceSelectorRace = FaceSelector::Race::AllRace;
        } else {
            m_logger->warn("[Config] Invalid face selector race: " + value + " Use default: All");
            m_faceSelectorRace = FaceSelector::Race::AllRace;
        }
    } else {
        m_faceSelectorRace = FaceSelector::Race::AllRace;
    }

    value = m_ini.GetValue("face_selector", "reference_face_position", "0");
    if (!value.empty()) {
        m_referenceFacePosition = std::stoi(value);
        if (m_referenceFacePosition < 0) {
            m_referenceFacePosition = 0;
        }
    } else {
        m_referenceFacePosition = 0;
    }

    value = m_ini.GetValue("face_selector", "reference_face_distance", "0.6");
    if (!value.empty()) {
        m_referenceFaceDistance = std::stof(value);
        if (m_referenceFaceDistance < 0) {
            m_referenceFaceDistance = 0;
        } else if (m_referenceFaceDistance > 1.5) {
            m_referenceFaceDistance = 1.5;
        }
    } else {
        m_referenceFaceDistance = 0.6f;
    }
    value = m_ini.GetValue("face_selector", "reference_frame_number", "0");
    if (!value.empty()) {
        m_referenceFrameNumber = std::stoi(value);
        if (m_referenceFrameNumber < 0) {
            m_referenceFrameNumber = 0;
        }
    } else {
        m_referenceFrameNumber = 0;
    }
}
void Config::faceAnalyser() {
    // face_analyser
    std::string value = m_ini.GetValue("face_analyser", "face_detector_model", "yoloface");
    std::vector<std::string> analysers;
    std::istringstream iss(value);
    std::string word;
    while (iss >> word) {
        analysers.push_back(word);
    }
    if (!analysers.empty()) {
        for (const auto &analyser : analysers) {
            if (analyser == "many") {
                m_faceDetectorModel = FaceDetectors::FaceDetectorType::Many;
            } else if (analyser == "retinaface") {
                m_faceDetectorModel = FaceDetectors::FaceDetectorType::Retina;
            } else if (analyser == "yoloface") {
                m_faceDetectorModel = FaceDetectors::FaceDetectorType::Yolo;
            } else if (analyser == "scrfd") {
                m_faceDetectorModel = FaceDetectors::FaceDetectorType::Scrfd;
            } else {
                m_logger->warn("[Config] Invalid face_analyser_model value: " + analyser + " Use default: yolo");
                m_faceDetectorModel = FaceDetectors::FaceDetectorType::Yolo;
            }
        }
    } else {
        m_logger->warn("[Config] face_analyser_model is not set. Use default: yolo");
        m_faceDetectorModel = FaceDetectors::FaceDetectorType::Yolo;
    }

    value = m_ini.GetValue("face_analyser", "face_detector_size", "640x640");
    if (!value.empty()) {
        m_faceDetectorSize = Vision::unpackResolution(value);
        if (m_faceDetectorSize.width < 0) {
            m_faceDetectorSize.width = 0;
        } else if (m_faceDetectorSize.width > 1024) {
            m_faceDetectorSize.width = 1024;
        }
        if (m_faceDetectorSize.height < 0) {
            m_faceDetectorSize.height = 0;
        } else if (m_faceDetectorSize.height > 1024) {
            m_faceDetectorSize.height = 1024;
        }
    } else {
        m_faceDetectorSize = cv::Size(640, 640);
    }

    value = m_ini.GetValue("face_analyser", "face_detector_score", "0.5");
    if (!value.empty()) {
        m_faceDetectorScore = std::stof(value);
        if (m_faceDetectorScore < 0.0f) {
            m_faceDetectorScore = 0.0f;
        } else if (m_faceDetectorScore > 1.0f) {
            m_faceDetectorScore = 1.0f;
        }
    } else {
        m_faceDetectorScore = 0.5f;
    }

    value = m_ini.GetValue("face_analyser", "face_landmarker_model", "2dfan4");
    tolower(value);
    if (!value.empty()) {
        if (value == "many") {
            m_faceLandmarkerModel = FaceLandmarkers::Landmarker68Model::Many;
        } else if (value == "2dfan4") {
            m_faceLandmarkerModel = FaceLandmarkers::Landmarker68Model::_2DFAN;
        } else if (value == "peppa_wutz") {
            m_faceLandmarkerModel = FaceLandmarkers::Landmarker68Model::PEPPA_WUTZ;
        } else {
            m_logger->warn("[Config] Invalid face_landmaker_model value: " + value + " Use default: 2dfan4");
            m_faceLandmarkerModel = FaceLandmarkers::Landmarker68Model::_2DFAN;
        }
    } else {
        m_faceLandmarkerModel = FaceLandmarkers::Landmarker68Model::_2DFAN;
    }

    value = m_ini.GetValue("face_analyser", "face_landmaker_score", "0.5");
    if (!value.empty()) {
        m_faceLandmarkerScore = std::stof(value);
    } else {
        m_faceLandmarkerScore = 0.5f;
    }
}

void Config::general() {
    // general
    std::string value = m_ini.GetValue("general", "source_path", "");
    if (!value.empty()) {
        if (FileSystem::fileExists(value) && FileSystem::isFile(value)) {
            m_sourcePaths.insert(value);
        } else if (FileSystem::isDirectory(value)) {
            m_sourcePaths = FileSystem::listFilesInDirectory(value);
            if (m_sourcePaths.empty()) {
                m_logger->warn("[Config] source_path is an empty directory.");
            } else {
                m_sourcePaths = FileSystem::filterImagePaths(m_sourcePaths);
                if (m_sourcePaths.empty()) {
                    m_logger->warn("[Config] source_path does not contain any valid image files.");
                }
            }
        } else {
            m_logger->warn("[Config] source_path is not a valid path or directory.");
        }
    } else {
        m_logger->warn("[Config] source_path is not set.");
    }

    value = m_ini.GetValue("general", "target_path", "");
    if (!value.empty()) {
        if (FileSystem::fileExists(value) && FileSystem::isFile(value)) {
            m_targetPaths.insert(value);
        } else if (FileSystem::isDirectory(value)) {
            m_targetPaths = FileSystem::listFilesInDirectory(value);
        } else {
            m_logger->error("[Config] target_path is not a valid path or directory.");
            std::exit(1);
        }
    } else {
        m_logger->error("[Config] target_path is not set.");
        std::exit(1);
    }

    value = m_ini.GetValue("general", "reference_face_path", "");
    if (!value.empty()) {
        if (FileSystem::fileExists(value) && FileSystem::isFile(value) && FileSystem::isImage(value)) {
            m_referenceFacePath = value;
            m_faceSelectorMode = FaceSelector::SelectorMode::Reference;
        } else {
            m_logger->warn("[Config] reference_face_path is not a valid path or file.");
            m_referenceFacePath = "";
        }
    } else {
        m_referenceFacePath = "";
    }

    value = m_ini.GetValue("general", "output_path", "./output");
    if (!value.empty()) {
        m_outputPath = FileSystem::absolutePath(value);
    } else {
        m_outputPath = FileSystem::absolutePath("./output");
        m_logger->warn("[Config] output_path is not set. Use default: " + m_outputPath);
    }
}

void Config::misc() {
    std::string value = m_ini.GetValue("misc", "force_download", "true");
    if (!value.empty()) {
        if (value == "true") {
            m_forceDownload = true;
        } else if (value == "false") {
            m_forceDownload = false;
        } else {
            m_logger->warn("[Config] Invalid force_download: " + value + " Use default: false");
            m_forceDownload = true;
        }
    } else {
        m_forceDownload = true;
    }

    value = m_ini.GetValue("misc", "skip_download", "false");
    if (!value.empty()) {
        if (value == "true") {
            m_skipDownload = true;
        } else if (value == "false") {
            m_skipDownload = false;
        } else {
            m_logger->warn("[Config] Invalid skip_download: " + value + " Use default: false");
            m_skipDownload = false;
        }
    } else {
        m_skipDownload = false;
    }

    value = m_ini.GetValue("misc", "log_level", "info");
    if (!value.empty()) {
        if (value == "trace") {
            m_logLevel = Logger::LogLevel::Trace;
        } else if (value == "debug") {
            m_logLevel = Logger::LogLevel::Debug;
        } else if (value == "info") {
            m_logLevel = Logger::LogLevel::Info;
        } else if (value == "warn") {
            m_logLevel = Logger::LogLevel::Warn;
        } else if (value == "error") {
            m_logLevel = Logger::LogLevel::Error;
        } else if (value == "critical") {
            m_logLevel = Logger::LogLevel::Critical;
        } else {
            m_logger->warn("[Config] Invalid log_level: " + value + " Use default: info");
            m_logLevel = Logger::LogLevel::Info;
        }
    } else {
        m_logLevel = Logger::LogLevel::Info;
    }
}

std::shared_ptr<Config> Config::getInstance(const std::string &configPath) {
    static std::shared_ptr<Config> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<Config>(configPath); });
    return instance;
}

void Config::execution() {
    std::string value = m_ini.GetValue("execution", "execution_device_id", "0");
    if (!value.empty()) {
        m_executionDeviceId = std::stoi(value);
        if (m_executionDeviceId < 0) {
            m_executionDeviceId = 0;
        }
    } else {
        m_executionDeviceId = 0;
    }

    value = m_ini.GetValue("execution", "execution_providers", "cpu");
    tolower(value);
    if (!value.empty()) {
        bool flag = false;
        if (value.find("cpu") != std::string::npos) {
            m_executionProviders.insert(Ffc::InferenceSession::ExecutionProvider::CPU);
            flag = true;
        }
        if (value.find("cuda") != std::string::npos) {
            m_executionProviders.insert(Ffc::InferenceSession::ExecutionProvider::CUDA);
            flag = true;
        }
        if (value.find("tensorrt") != std::string::npos) {
            m_executionProviders.insert(Ffc::InferenceSession::ExecutionProvider::TensorRT);
            flag = true;
        }
        if (!flag) {
            m_logger->warn("[Config] Invalid execution_providers: " + value + " Use default: cpu");
            m_executionProviders.insert(Ffc::InferenceSession::ExecutionProvider::CPU);
        }
    } else {
        m_executionProviders.insert(Ffc::InferenceSession::ExecutionProvider::CPU);
    }

    value = m_ini.GetValue("execution", "execution_thread_count", "1");
    if (!value.empty()) {
        m_executionThreadCount = std::stoi(value);
        if (m_executionThreadCount < 1) {
            m_executionThreadCount = 1;
        }
    } else {
        m_executionThreadCount = 1;
    }
}

void Config::tensort() {
    std::string value = m_ini.GetValue("tensorrt", "enable_engine_cache", "true");
    tolower(value);
    if (!value.empty()) {
        if (value == "true") {
            m_enableTensorrtCache = true;
        } else if (value == "false") {
            m_enableTensorrtCache = false;
        } else {
            m_logger->warn("[Config] Invalid enable_cache: " + value + " Use default: true");
            m_enableTensorrtCache = true;
        }
    } else {
        m_enableTensorrtCache = true;
    }

    value = m_ini.GetValue("tensorrt", "enable_embed_engine", "true");
    tolower(value);
    if (!value.empty()) {
        if (value == "true") {
            m_enableTensorrtEmbedEngine = true;
        } else if (value == "false") {
            m_enableTensorrtEmbedEngine = false;
        } else {
            m_logger->warn("[Config] Invalid enable_embed_engine: " + value + " Use default: true");
            m_enableTensorrtEmbedEngine = true;
        }
    } else {
        m_enableTensorrtEmbedEngine = true;
    }

    value = m_ini.GetValue("tensorrt", "per_session_gpu_mem_limit", "0");
    if (!value.empty()) {
        float gb = std::stof(value);
        if (gb < 0) {
            gb = 0;
        }
        m_trtMaxWorkspaceSize = (size_t)(gb * (float)(1 << 30));
    } else {
        m_trtMaxWorkspaceSize = 0;
    }
}

void Config::memory() {
    std::string value = m_ini.GetValue("memory", "processor_memory_strategy", "moderate");
    tolower(value);
    if (!value.empty()) {
        if (value == "strict") {
            m_processorMemoryStrategy = ProcessorMemoryStrategy::Strict;
        } else if (value == "tolerant") {
            m_processorMemoryStrategy = ProcessorMemoryStrategy::Tolerant;
        } else {
            m_logger->warn("[Config] Invalid processor_memory_strategy: " + value + " Use default: tolerant");
            m_processorMemoryStrategy = ProcessorMemoryStrategy::Tolerant;
        }
    } else {
        m_processorMemoryStrategy = ProcessorMemoryStrategy::Tolerant;
    }
}

void Config::video() {
    std::string value = m_ini.GetValue("video", "video_segment_duration", "0");
    if (!value.empty()) {
        m_videoSegmentDuration = std::stoi(value);
        if (m_videoSegmentDuration < 0) {
            m_videoSegmentDuration = 0;
        }
    } else {
        m_videoSegmentDuration = 0;
    }

    value = m_ini.GetValue("video", "output_video_encoder", "libx264");
    const std::unordered_set<std::string> encoders = {"libx264", "libx265", "libvpx-vp9", "h264_nvenc", "hevc_nvenc", "h264_amf", "hevc_amf"};
    if (!value.empty()) {
        if (encoders.contains(value)) {
            m_outputVideoEncoder = value;
        } else {
            m_logger->warn("[Config] Invalid output_video_encoder: " + value + " Use default: libx264");
            m_outputVideoEncoder = "libx264";
        }
    } else {
        m_outputVideoEncoder = "libx264";
    }

    value = m_ini.GetValue("video", "output_video_preset", "veryfast");
    const std::unordered_set<std::string> presets = {"ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow"};
    if (!value.empty()) {
        if (presets.contains(value)) {
            m_outputVideoPreset = value;
        } else {
            m_logger->warn("[Config] Invalid output_video_preset: " + value + " Use default: veryfast");
            m_outputVideoPreset = "veryfast";
        }
    } else {
        m_outputVideoPreset = "veryfast";
    }

    value = m_ini.GetValue("video", "output_video_quality", "80");
    if (!value.empty()) {
        m_outputVideoQuality = std::stoi(value);
        if (m_outputVideoQuality < 0) {
            m_outputVideoQuality = 0;
        } else if (m_outputVideoQuality > 100) {
            m_outputVideoQuality = 100;
        }
    } else {
        m_outputVideoQuality = 80;
    }

    value = m_ini.GetValue("video", "output_audio_encoder", "aac");
    const std::unordered_set<std::string> audioEncoders = {"aac", "libmp3lame", "libopus", "libvorbis"};
    if (!value.empty()) {
        if (audioEncoders.contains(value)) {
            m_outputAudioEncoder = value;
        } else {
            m_logger->warn("[Config] Invalid output_audio_encoder: " + value + " Use default: aac");
            m_outputAudioEncoder = "aac";
        }
    } else {
        m_outputAudioEncoder = "aac";
    }

    value = m_ini.GetValue("video", "skip_audio", "false");
    if (!value.empty()) {
        if (value == "true") {
            m_skipAudio = true;
        } else if (value == "false") {
            m_skipAudio = false;
        } else {
            m_logger->warn("[Config] Invalid skip_audio: " + value + " Use default: false");
            m_skipAudio = false;
        }
    } else {
        m_skipAudio = false;
    }

    value = m_ini.GetValue("video", "temp_frame_format", "png");
    const std::unordered_set<std::string> formats = {"png", "jpg", "bmp"};
    if (!value.empty()) {
        if (formats.contains(value)) {
            m_tempFrameFormat = value;
        } else {
            m_logger->warn("[Config] Invalid temp_frame_format: " + value + " Use default: png");
            m_tempFrameFormat = "png";
        }
    } else {
        m_tempFrameFormat = "png";
    }
}

void Config::tolower(std::string &str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}
}; // namespace Ffc