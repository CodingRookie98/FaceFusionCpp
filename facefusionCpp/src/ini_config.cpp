/**
 ******************************************************************************
 * @file           : config.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-17
 ******************************************************************************
 */

module;
#include <regex>
#include <unordered_set>
#include <SimpleIni.h>
#include <opencv2/opencv.hpp>

module ini_config;
import core_options;
import file_system;
import vision;
import face_selector;
import face_masker_hub;
import face_detector_hub;
import face_landmarker_hub;
import inference_session;
import face_selector;
import face_analyser;
import model_manager;

namespace ffc {
using namespace faceMasker;
using namespace faceLandmarker;
using namespace faceDetector;

ini_config::ini_config() = default;

bool ini_config::loadConfig(const std::string& configPath) {
    if (!configPath.empty() && FileSystem::fileExists(configPath)) {
        config_path_ = configPath;
    } else {
        logger_->error(std::format("IniConfig file not found: {}", FileSystem::absolutePath(configPath)));
        return false;
    }

    std::unique_lock lock(shared_mutex_);

    ini_.SetUnicode();
    if (const SI_Error rc = ini_.LoadFile(config_path_.c_str()); rc < 0) {
        logger_->error("Failed to load config file");
        throw std::runtime_error("Failed to load config file");
    }

    frameProcessors();
    paths();
    misc();
    execution();
    tensorrt();
    memory();
    faceAnalyser();
    faceSelector();
    faceMasker();
    image();
    video();
    return true;
}

std::array<int, 4> ini_config::normalizePadding(const std::vector<int>& padding) {
    if (padding.size() == 1) {
        return {padding[0], padding[0], padding[0], padding[0]};
    }
    if (padding.size() == 2) {
        return {padding[0], padding[1], padding[0], padding[1]};
    }
    if (padding.size() == 3) {
        return {padding[0], padding[1], padding[2], padding[1]};
    }
    if (padding.size() == 4) {
        return {padding[0], padding[1], padding[2], padding[3]};
    }
    throw std::invalid_argument("Invalid padding length");
}

std::vector<int> ini_config::parseStr2VecInt(const std::string& input) {
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

void ini_config::frameProcessors() {
    // frame_processors
    std::string value = ini_.GetValue("frame_processors", "frame_processors");
    tolower(value);
    if (!value.empty()) {
        std::vector<std::string> result;

        // 使用正则表达式分割字符串，匹配任意多个空格
        const std::regex re("\\s+");
        std::sregex_token_iterator iter(value.begin(), value.end(), re, -1);
        std::sregex_token_iterator end;

        // 将分割的字符串存入vector
        for (; iter != end; ++iter) {
            if (!iter->str().empty()) {
                result.push_back(*iter);
            }
        }
        bool flag = false;
        for (const auto& item : result) {
            if (item == "face_swapper") {
                core_task_.processor_list.emplace_back(ProcessorMajorType::FaceSwapper);
                flag = true;
            }
            if (item == "face_enhancer") {
                core_task_.processor_list.emplace_back(ProcessorMajorType::FaceEnhancer);
                flag = true;
            }
            if (item == "expression_restorer") {
                core_task_.processor_list.emplace_back(ProcessorMajorType::ExpressionRestorer);
                flag = true;
            }
            if (item == "frame_enhancer") {
                core_task_.processor_list.emplace_back(ProcessorMajorType::FrameEnhancer);
                flag = true;
            }
        }
        if (!flag) {
            logger_->warn("[IniConfig] The user-specified frame processors are not supported;");
            std::exit(1);
        }
    } else {
        logger_->error("[IniConfig] No frame processors specified.");
        std::exit(1);
    }
    if (core_task_.processor_list.empty()) {
        logger_->error("[IniConfig] No frame processors specified.");
        std::exit(1);
    }

    if (std::ranges::find(core_task_.processor_list.begin(), core_task_.processor_list.end(), ProcessorMajorType::FaceEnhancer)
        != core_task_.processor_list.end()) {
        value = ini_.GetValue("frame_processors", "face_enhancer_model", "gfpgan_1.4");
        tolower(value);
        if (value == "codeformer") {
            core_task_.processor_model[ProcessorMajorType::FaceEnhancer] = ModelManager::Model::Codeformer;
            core_task_.processor_minor_types[ProcessorMajorType::FaceEnhancer] = ProcessorMinorType{.face_enhancer = FaceEnhancerType::CodeFormer};
        } else if (value == "gfpgan_1.2") {
            core_task_.processor_model[ProcessorMajorType::FaceEnhancer] = ModelManager::Model::Gfpgan_12;
            core_task_.processor_minor_types[ProcessorMajorType::FaceEnhancer] = ProcessorMinorType{.face_enhancer = FaceEnhancerType::GFP_GAN};
        } else if (value == "gfpgan_1.3") {
            core_task_.processor_model[ProcessorMajorType::FaceEnhancer] = ModelManager::Model::Gfpgan_13;
            core_task_.processor_minor_types[ProcessorMajorType::FaceEnhancer] = ProcessorMinorType{.face_enhancer = FaceEnhancerType::GFP_GAN};
        } else if (value == "gfpgan_1.4") {
            core_task_.processor_model[ProcessorMajorType::FaceEnhancer] = ModelManager::Model::Gfpgan_14;
            core_task_.processor_minor_types[ProcessorMajorType::FaceEnhancer] = ProcessorMinorType{.face_enhancer = FaceEnhancerType::GFP_GAN};
        } else {
            logger_->warn(std::format("Invalid face enhancer model: {}, Use Default: gfpgan_1.4", value));
            core_task_.processor_model[ProcessorMajorType::FaceEnhancer] = ModelManager::Model::Gfpgan_14;
            core_task_.processor_minor_types[ProcessorMajorType::FaceEnhancer] = ProcessorMinorType{.face_enhancer = FaceEnhancerType::GFP_GAN};
        }

        unsigned short face_enhancer_blend = ini_.GetLongValue("frame_processors", "face_enhancer_blend", 80);
        if (face_enhancer_blend < 0) {
            face_enhancer_blend = 0;
        }
        if (face_enhancer_blend > 100) {
            face_enhancer_blend = 100;
        }
        core_task_.face_enhancer_blend = face_enhancer_blend;
    }

    if (std::ranges::find(core_task_.processor_list.begin(), core_task_.processor_list.end(), ProcessorMajorType::FaceSwapper)
        != core_task_.processor_list.end()) {
        value = ini_.GetValue("frame_processors", "face_swapper_model", "inswapper_128_fp16");
        tolower(value);
        if (value == "inswapper_128_fp16") {
            core_task_.processor_model[ProcessorMajorType::FaceSwapper] = ModelManager::Model::Inswapper_128_fp16;
            core_task_.processor_minor_types[ProcessorMajorType::FaceSwapper] = ProcessorMinorType{.face_swapper = FaceSwapperType::InSwapper};
        } else if (value == "inswapper_128") {
            core_task_.processor_model[ProcessorMajorType::FaceSwapper] = ModelManager::Model::Inswapper_128;
            core_task_.processor_minor_types[ProcessorMajorType::FaceSwapper] = ProcessorMinorType{.face_swapper = FaceSwapperType::InSwapper};
        } else {
            logger_->warn(std::format("[IniConfig] Invalid face swapper model: {}, Use Default: inswapper_128_fp16", value));
            core_task_.processor_model[ProcessorMajorType::FaceSwapper] = ModelManager::Model::Inswapper_128_fp16;
            core_task_.processor_minor_types[ProcessorMajorType::FaceSwapper] = ProcessorMinorType{.face_swapper = FaceSwapperType::InSwapper};
        }
    }

    if (std::ranges::find(core_task_.processor_list.begin(), core_task_.processor_list.end(), ProcessorMajorType::ExpressionRestorer)
        != core_task_.processor_list.end()) {
        value = ini_.GetValue("frame_processors", "expression_restorer_model", "live_portrait");
        tolower(value);
        if (value == "live_portrait") {
            core_task_.processor_minor_types[ProcessorMajorType::ExpressionRestorer] = ProcessorMinorType{.expression_restorer = ExpressionRestorerType::LivePortrait};
        } else {
            logger_->warn(std::format("[IniConfig] Invalid expression restorer model: {}, Use Default: live_portrait", value));
            core_task_.processor_minor_types[ProcessorMajorType::ExpressionRestorer] = ProcessorMinorType{.expression_restorer = ExpressionRestorerType::LivePortrait};
        }

        unsigned short expression_restorer_factor = ini_.GetLongValue("frame_processors", "expression_restorer_factor", 80);
        if (expression_restorer_factor < 0) {
            expression_restorer_factor = 0;
        }
        if (expression_restorer_factor > 100) {
            expression_restorer_factor = 100;
        }
        expression_restorer_factor = expression_restorer_factor / 100.0f * 1.2f;
        core_task_.expression_restorer_factor = expression_restorer_factor;
    }

    if (std::ranges::find(core_task_.processor_list.begin(), core_task_.processor_list.end(), ProcessorMajorType::FrameEnhancer)
        != core_task_.processor_list.end()) {
        value = ini_.GetValue("frame_processors", "frame_enhancer_model", "real_hatgan_x4");
        tolower(value);
        if (value == "real_esrgan_x2") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x2;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_esrgan_x2_fp16") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x2_fp16;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_esrgan_x4") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x4;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_esrgan_x4_fp16") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x4_fp16;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_esrgan_x8") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x8;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_esrgan_x8_fp16") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_esrgan_x8_fp16;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_esr_gan};
        } else if (value == "real_hatgan_x4") {
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_hatgan_x4;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_hat_gan};
        } else {
            logger_->warn(std::format("[IniConfig] Invalid frame enhancer: {}, Use Default: real_hatgan_x4", value));
            core_task_.processor_model[ProcessorMajorType::FrameEnhancer] = ModelManager::Model::Real_hatgan_x4;
            core_task_.processor_minor_types[ProcessorMajorType::FrameEnhancer] = {.frame_enhancer = FrameEnhancerType::Real_hat_gan};
        }

        unsigned short frame_enhancer_blend = ini_.GetLongValue("frame_processors", "frame_enhancer_blend", 80);
        if (frame_enhancer_blend < 0) {
            frame_enhancer_blend = 0;
        }
        if (frame_enhancer_blend > 100) {
            frame_enhancer_blend = 100;
        }
        core_task_.frame_enhancer_blend = frame_enhancer_blend;
    }
}

void ini_config::image() {
    // output_creation
    unsigned short output_image_quality = ini_.GetLongValue("image", "output_image_quality", 100);
    if (output_image_quality < 0) {
        output_image_quality = 0;
    }
    if (output_image_quality > 100) {
        output_image_quality = 100;
    }
    core_task_.output_image_quality = output_image_quality;

    cv::Size output_image_size;
    if (const std::string value = ini_.GetValue("image", "output_image_resolution", "");
        !value.empty()) {
        output_image_size = vision::unpackResolution(value);
    } else {
        output_image_size = cv::Size(0, 0);
    }
    core_task_.output_image_size = output_image_size;
}

void ini_config::faceMasker() {
    const std::string section_name{"face_masker"};
    std::string value = ini_.GetValue(section_name.c_str(), "face_mask_types", "box");
    if (!value.empty()) {
        core_task_.face_mask_types = std::make_optional<std::unordered_set<FaceMaskerHub::Type>>();
        if (value.find("box") != std::string::npos) {
            core_task_.face_mask_types->insert(FaceMaskerHub::Type::Box);
        }
        if (value.find("occlusion") != std::string::npos) {
            core_task_.face_mask_types->insert(FaceMaskerHub::Type::Occlusion);
        }
        if (value.find("region") != std::string::npos) {
            core_task_.face_mask_types->insert(FaceMaskerHub::Type::Region);
        }
    } else {
        core_task_.face_mask_types->insert(FaceMaskerHub::Type::Box);
    }

    if (core_task_.face_mask_types.value().contains(FaceMaskerHub::Type::Occlusion)) {
        value = ini_.GetValue(section_name.c_str(), "face_occluder_model", "xseg_1");
        if (value == "xseg_1") {
            core_task_.face_occluder_model = ModelManager::Model::xseg_1;
        } else if (value == "xseg_2") {
            core_task_.face_occluder_model = ModelManager::Model::xseg_2;
        } else {
            core_task_.face_occluder_model = ModelManager::Model::xseg_1;
        }
    }

    if (core_task_.face_mask_types.value().contains(FaceMaskerHub::Type::Region)) {
        value = ini_.GetValue(section_name.c_str(), "face_parser_model", "bisenet_resnet_34");
        if (value == "bisenet_resnet_34") {
            core_task_.face_parser_model = ModelManager::Model::bisenet_resnet_34;
        } else if (value == "bisenet_resnet_18") {
            core_task_.face_parser_model = ModelManager::Model::bisenet_resnet_18;
        } else {
            core_task_.face_parser_model = ModelManager::Model::bisenet_resnet_34;
        }
    }

    core_task_.face_mask_blur = ini_.GetDoubleValue("face_mask", "face_mask_blur", 0.3);
    if (core_task_.face_mask_blur < 0) {
        core_task_.face_mask_blur = 0;
    } else if (core_task_.face_mask_blur > 1) {
        core_task_.face_mask_blur = 1;
    }

    value = ini_.GetValue("face_masker", "face_mask_padding", "0 0 0 0");
    if (!value.empty()) {
        core_task_.face_mask_padding = normalizePadding(parseStr2VecInt(value));
    } else {
        core_task_.face_mask_padding = {0, 0, 0, 0};
    }

    value = ini_.GetValue("face_masker", "face_mask_region", "All");
    tolower(value);
    if (value == "all") {
        core_task_.face_mask_regions = FaceMaskerRegion::getAllRegions();
    } else if (value == "skin") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::Skin);
    } else if (value == "nose") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::Nose);
    } else if (value == "left-eyebrow") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::LeftEyebrow);
    } else if (value == "right-eyebrow") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::RightEyebrow);
    } else if (value == "mouth") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::Mouth);
    } else if (value == "right-eye") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::RightEye);
    } else if (value == "left-eye") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::LeftEye);
    } else if (value == "glasses") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::Glasses);
    } else if (value == "upper-lip") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::UpperLip);
    } else if (value == "lower-lip") {
        core_task_.face_mask_regions->insert(FaceMaskerRegion::Region::LowerLip);
    } else {
        logger_->warn("[IniConfig] Invalid face mask region: " + value + " Use default: All");
        core_task_.face_mask_regions = FaceMaskerRegion::getAllRegions();
    }
}
void ini_config::faceSelector() {
    // face_selector
    std::string value = ini_.GetValue("face_selector", "face_selector_mode", "reference");
    if (!value.empty() && core_task_.reference_face_path->empty()) {
        if (value == "reference") {
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Reference;
        } else if (value == "one") {
            core_task_.face_selector_mode = FaceSelector::SelectorMode::One;
        } else if (value == "many") {
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Many;
        } else {
            logger_->warn("[IniConfig] Invalid face selector mode: " + value + " Use default: reference");
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Many;
        }
    } else {
        if (core_task_.reference_face_path.has_value() && !core_task_.reference_face_path->empty()) {
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Reference;
        } else {
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Many;
        }
    }

    value = ini_.GetValue("face_selector", "face_selector_order", "left-right");
    tolower(value);
    FaceSelector::Options face_selector_options;
    if (!value.empty()) {
        if (value == "left-right") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Left_Right;
        } else if (value == "right-left") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Right_Left;
        } else if (value == "top-bottom") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Top_Bottom;
        } else if (value == "bottom-top") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Bottom_Top;
        } else if (value == "smAll-large") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Small_Large;
        } else if (value == "large-smAll") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Large_Small;
        } else if (value == "best-worst") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Best_Worst;
        } else if (value == "worst-best") {
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Worst_Best;
        } else {
            logger_->warn("[IniConfig] Invalid face selector order: " + value + " Use default: left-right");
            face_selector_options.order = FaceSelector::FaceSelectorOrder::Left_Right;
        }
    } else {
        face_selector_options.order = FaceSelector::FaceSelectorOrder::Left_Right;
    }

    face_selector_options.ageStart = ini_.GetLongValue("face_selector", "face_selector_age_start", 0);
    if (face_selector_options.ageStart < 0) {
        face_selector_options.ageStart = 0;
    }
    if (face_selector_options.ageStart > 100) {
        face_selector_options.ageStart = 70;
    }

    face_selector_options.ageEnd = ini_.GetLongValue("face_selector", "face_selector_age_end", 100);
    if (face_selector_options.ageEnd < 0 || face_selector_options.ageEnd > 100) {
        face_selector_options.ageEnd = 100;
    }
    core_task_.face_analyser_options->faceSelectorOptions = face_selector_options;

    core_task_.reference_face_position = ini_.GetLongValue("face_selector", "reference_face_position", 0);
    if (core_task_.reference_face_position < 0) {
        core_task_.reference_face_position = 0;
    }

    core_task_.reference_face_distance = ini_.GetDoubleValue("face_selector", "reference_face_distance", 0.6);
    if (core_task_.reference_face_distance < 0) {
        core_task_.reference_face_distance = 0;
    } else if (core_task_.reference_face_distance > 1.5) {
        core_task_.reference_face_distance = 1.5;
    }

    core_task_.reference_frame_number = ini_.GetLongValue("face_selector", "reference_frame_number", 0);
    if (core_task_.reference_frame_number < 0) {
        core_task_.reference_frame_number = 0;
    }
}

void ini_config::faceAnalyser() {
    // face_analyser
    std::string value = ini_.GetValue("face_analyser", "face_detector_model", "yoloface");
    std::vector<std::string> detectorModel;
    std::istringstream iss(value);
    std::string word;
    while (iss >> word) {
        detectorModel.push_back(word);
    }
    FaceDetectorHub::Options face_detector_options;
    if (!detectorModel.empty()) {
        for (const auto& analyser : detectorModel) {
            if (analyser == "many") {
                face_detector_options.types.insert({FaceDetectorHub::Type::Retina, FaceDetectorHub::Type::Yolo, FaceDetectorHub::Type::Scrfd});
                break;
            }
            if (analyser == "retinaface") {
                face_detector_options.types.insert(FaceDetectorHub::Type::Retina);
            } else if (analyser == "yoloface") {
                face_detector_options.types.insert(FaceDetectorHub::Type::Yolo);
            } else if (analyser == "scrfd") {
                face_detector_options.types.insert(FaceDetectorHub::Type::Scrfd);
            } else {
                logger_->warn("[IniConfig] Invalid face_analyser_model value: " + analyser + " Use default: yolo");
                face_detector_options.types.insert(FaceDetectorHub::Type::Yolo);
            }
        }
    } else {
        logger_->warn("[IniConfig] face_analyser_model is not set. Use default: yolo");
        face_detector_options.types.insert(FaceDetectorHub::Type::Yolo);
    }

    value = ini_.GetValue("face_analyser", "face_detector_size", "640x640");
    if (!value.empty()) {
        cv::Size faceDetectorSize = vision::unpackResolution(value);
        const auto supportCommonSizes = FaceDetectorHub::GetSupportCommonSizes(face_detector_options.types);
        bool isIn = false;
        cv::Size maxSize{0, 0};
        for (const auto& commonSize : supportCommonSizes) {
            if (commonSize == faceDetectorSize) {
                isIn = true;
                break;
            }
            if (commonSize.area() > maxSize.area()) {
                maxSize = commonSize;
            }
        }
        if (!isIn) {
            faceDetectorSize = maxSize;
        }

        face_detector_options.face_detector_size = faceDetectorSize;
    }

    face_detector_options.min_score = ini_.GetDoubleValue("face_analyser", "face_detector_score", 0.5);
    if (face_detector_options.min_score < 0.0f) {
        face_detector_options.min_score = 0.0f;
    } else if (face_detector_options.min_score > 1.0f) {
        face_detector_options.min_score = 1.0f;
    }
    core_task_.face_analyser_options = {FaceAnalyser::Options{}};
    core_task_.face_analyser_options->faceDetectorOptions = face_detector_options;

    value = ini_.GetValue("face_analyser", "face_landmarker_model", "2dfan4");
    FaceLandmarkerHub::Options face_landmarker_options;
    tolower(value);
    if (!value.empty()) {
        if (value == "many") {
            face_landmarker_options.types.insert(FaceLandmarkerHub::Type::_2DFAN);
            face_landmarker_options.types.insert(FaceLandmarkerHub::Type::PEPPA_WUTZ);
        } else if (value == "2dfan4") {
            face_landmarker_options.types.insert(FaceLandmarkerHub::Type::_2DFAN);
        } else if (value == "peppa_wutz") {
            face_landmarker_options.types.insert(FaceLandmarkerHub::Type::PEPPA_WUTZ);
        } else {
            logger_->warn("[IniConfig] Invalid face_landmaker_model value: " + value + " Use default: 2dfan4");
            face_landmarker_options.types.insert(FaceLandmarkerHub::Type::_2DFAN);
        }
    } else {
        face_landmarker_options.types.insert(FaceLandmarkerHub::Type::_2DFAN);
    }

    face_landmarker_options.minScore = ini_.GetDoubleValue("face_analyser", "face_landmaker_score", 0.5);
    if (face_landmarker_options.minScore < 0.0f) {
        face_landmarker_options.minScore = 0.0f;
    } else if (face_landmarker_options.minScore > 1.0f) {
        face_landmarker_options.minScore = 1.0f;
    }
    core_task_.face_analyser_options->faceLandMarkerOptions = face_landmarker_options;
}

void ini_config::paths() {
    std::string value;
    if (std::ranges::find(core_task_.processor_list, ProcessorMajorType::FaceSwapper) != core_task_.processor_list.end()
        || std::ranges::find(core_task_.processor_list, ProcessorMajorType::ExpressionRestorer) != core_task_.processor_list.end()) {
        value = ini_.GetValue("paths", "source_path", "");
        if (!value.empty()) {
            core_task_.source_paths = std::make_optional(std::vector<std::string>{});
            if (FileSystem::fileExists(value) && FileSystem::isFile(value)) {
                core_task_.source_paths->emplace_back(value);
            } else if (FileSystem::isDir(value)) {
                std::unordered_set<std::string> filePaths = FileSystem::listFilesInDir(value);
                for (const auto& filePath : filePaths) {
                    core_task_.source_paths->emplace_back(filePath);
                }
            } else {
                logger_->warn("[IniConfig] source_path is not a valid path or directory.");
            }
        } else {
            logger_->warn("[IniConfig] source_path is not set.");
        }
    }

    value = ini_.GetValue("paths", "target_path", "");
    if (!value.empty()) {
        if (FileSystem::isFile(value)) {
            core_task_.target_paths.emplace_back(value);
        } else if (FileSystem::isDir(value)) {
            std::unordered_set<std::string> filePaths = FileSystem::listFilesInDir(value);
            for (const auto& filePath : filePaths) {
                core_task_.target_paths.emplace_back(filePath);
            }
        } else {
            logger_->error("[IniConfig] target_path is not a valid path or directory.");
            std::exit(1);
        }
    } else {
        logger_->error("[IniConfig] target_path is not set.");
        std::exit(1);
    }

    value = ini_.GetValue("paths", "reference_face_path", "");
    if (!value.empty()) {
        if (FileSystem::fileExists(value) && FileSystem::isFile(value) && FileSystem::isImage(value)) {
            core_task_.reference_face_path = value;
            core_task_.face_selector_mode = FaceSelector::SelectorMode::Reference;
        } else {
            logger_->warn("[IniConfig] reference_face_path is not a valid path or file.");
        }
    }

    value = ini_.GetValue("paths", "output_path", "./output");
    std::string output_path;
    if (!value.empty()) {
        output_path = FileSystem::absolutePath(value);
    } else {
        output_path = FileSystem::absolutePath("./output");
        logger_->warn("[IniConfig] output_path is not set. Use default: " + output_path);
        FileSystem::createDir(output_path);
    }
    if (FileSystem::isFile(output_path)) {
        logger_->error("[IniConfig] output_path is a file. It must be a directory.");
        std::exit(1);
    }
    if (!FileSystem::dirExists(output_path)) {
        FileSystem::createDir(output_path);
    }
    core_task_.output_paths = FileSystem::normalizeOutputPaths(core_task_.target_paths, output_path);
}

void ini_config::misc() {
    core_options_.force_download = ini_.GetBoolValue("misc", "force_download", true);
    core_options_.skip_download = ini_.GetBoolValue("misc", "skip_download", false);

    std::string value = ini_.GetValue("misc", "log_level", "info");
    if (!value.empty()) {
        if (value == "trace") {
            core_options_.log_level = Logger::LogLevel::Trace;
        } else if (value == "debug") {
            core_options_.log_level = Logger::LogLevel::Debug;
        } else if (value == "info") {
            core_options_.log_level = Logger::LogLevel::Info;
        } else if (value == "warn") {
            core_options_.log_level = Logger::LogLevel::Warn;
        } else if (value == "error") {
            core_options_.log_level = Logger::LogLevel::Error;
        } else if (value == "critical") {
            core_options_.log_level = Logger::LogLevel::Critical;
        } else {
            logger_->warn("[IniConfig] Invalid log_level: " + value + " Use default: info");
            core_options_.log_level = Logger::LogLevel::Info;
        }
    } else {
        core_options_.log_level = Logger::LogLevel::Info;
    }
}

void ini_config::execution() {
    core_options_.inference_session_options.execution_device_id = ini_.GetLongValue("execution", "execution_device_id", 0);
    if (core_options_.inference_session_options.execution_device_id < 0) {
        core_options_.inference_session_options.execution_device_id = 0;
    }

    std::string value = ini_.GetValue("execution", "execution_providers", "cpu");
    tolower(value);
    if (!value.empty()) {
        bool flag = false;
        if (value.find("cpu") != std::string::npos) {
            core_options_.inference_session_options.execution_providers.insert(InferenceSession::ExecutionProvider::CPU);
            flag = true;
        }
        if (value.find("cuda") != std::string::npos) {
            core_options_.inference_session_options.execution_providers.insert(InferenceSession::ExecutionProvider::CUDA);
            flag = true;
        }
        if (value.find("tensorrt") != std::string::npos) {
            core_options_.inference_session_options.execution_providers.insert(InferenceSession::ExecutionProvider::TensorRT);
            flag = true;
        }
        if (!flag) {
            logger_->warn("[IniConfig] Invalid execution_providers: " + value + " Use default: cpu");
            core_options_.inference_session_options.execution_providers.insert(InferenceSession::ExecutionProvider::CPU);
        }
    } else {
        core_options_.inference_session_options.execution_providers.insert(InferenceSession::ExecutionProvider::CPU);
    }

    core_options_.execution_thread_count = ini_.GetLongValue("execution", "execution_thread_count", 1);
    if (core_options_.execution_thread_count < 1) {
        core_options_.execution_thread_count = 1;
    }
}

void ini_config::tensorrt() {
    core_options_.inference_session_options.enable_tensorrt_cache = ini_.GetBoolValue("tensorrt", "enable_engine_cache", true);
    core_options_.inference_session_options.enable_tensorrt_embed_engine = ini_.GetBoolValue("tensorrt", "enable_embed_engine", true);

    float gb = static_cast<float>(ini_.GetLongValue("tensorrt", "per_session_gpu_mem_limit", 0));
    if (gb < 0) {
        gb = 0;
    }
    core_options_.inference_session_options.trt_max_workspace_size = static_cast<size_t>(gb * static_cast<float>(1 << 30));
}

void ini_config::memory() {
    std::string value = ini_.GetValue("memory", "processor_memory_strategy", "moderate");
    tolower(value);
    CoreOptions::MemoryStrategy processorMemoryStrategy;
    if (!value.empty()) {
        if (value == "strict") {
            processorMemoryStrategy = CoreOptions::MemoryStrategy::Strict;
        } else if (value == "tolerant") {
            processorMemoryStrategy = CoreOptions::MemoryStrategy::Tolerant;
        } else {
            logger_->warn("[IniConfig] Invalid processor_memory_strategy: " + value + " Use default: tolerant");
            processorMemoryStrategy = CoreOptions::MemoryStrategy::Tolerant;
        }
    } else {
        processorMemoryStrategy = CoreOptions::MemoryStrategy::Tolerant;
    }
    core_options_.processor_memory_strategy = processorMemoryStrategy;
}

void ini_config::video() {
    unsigned long video_segment_duration = ini_.GetLongValue("video", "video_segment_duration", 0);
    core_task_.video_segment_duration = video_segment_duration;

    std::string value = ini_.GetValue("video", "output_video_encoder", "libx264");
    const std::unordered_set<std::string> encoders = {"libx264", "libx265", "libvpx-vp9", "h264_nvenc", "hevc_nvenc", "h264_amf", "hevc_amf"};
    std::string output_video_encoder;
    if (!value.empty()) {
        if (encoders.contains(value)) {
            output_video_encoder = value;
        } else {
            logger_->warn("[IniConfig] Invalid output_video_encoder: " + value + " Use default: libx264");
            output_video_encoder = "libx264";
        }
    } else {
        output_video_encoder = "libx264";
    }
    core_task_.output_video_encoder = output_video_encoder;

    value = ini_.GetValue("video", "output_video_preset", "veryfast");
    const std::unordered_set<std::string> presets = {"ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow"};
    std::string output_video_preset;
    if (!value.empty()) {
        if (presets.contains(value)) {
            output_video_preset = value;
        } else {
            logger_->warn("[IniConfig] Invalid output_video_preset: " + value + " Use default: veryfast");
            output_video_preset = "veryfast";
        }
    } else {
        output_video_preset = "veryfast";
    }
    core_task_.output_video_preset = output_video_preset;

    unsigned short output_video_quality = ini_.GetLongValue("video", "output_video_quality", 80);
    if (output_video_quality < 0) {
        output_video_quality = 0;
    }
    if (output_video_quality > 100) {
        output_video_quality = 100;
    }
    core_task_.output_video_quality = output_video_quality;

    value = ini_.GetValue("video", "output_audio_encoder", "aac");
    const std::unordered_set<std::string> audioEncoders = {"aac", "libmp3lame", "libopus", "libvorbis"};
    std::string output_audio_encoder;
    if (!value.empty()) {
        if (audioEncoders.contains(value)) {
            output_audio_encoder = value;
        } else {
            logger_->warn("[IniConfig] Invalid output_audio_encoder: " + value + " Use default: aac");
            output_audio_encoder = "aac";
        }
    } else {
        output_audio_encoder = "aac";
    }
    core_task_.output_audio_encoder = output_audio_encoder;

    ini_.GetBoolValue("video", "skip_audio", false);

    value = ini_.GetValue("video", "temp_frame_format", "png");
    const std::unordered_set<std::string> formats = {"png", "jpg", "bmp"};
    std::string temp_frame_format;
    if (!value.empty()) {
        if (formats.contains(value)) {
            temp_frame_format = value;
        } else {
            logger_->warn("[IniConfig] Invalid temp_frame_format: " + value + " Use default: png");
            temp_frame_format = "png";
        }
    } else {
        temp_frame_format = "png";
    }
    core_task_.temp_frame_format = temp_frame_format;
}

void ini_config::tolower(std::string& str) {
    std::ranges::transform(str, str.begin(),
                           [](const unsigned char c) { return std::tolower(c); });
}

}; // namespace ffc