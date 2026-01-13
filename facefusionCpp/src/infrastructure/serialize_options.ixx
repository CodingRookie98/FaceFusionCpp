module;
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

export module serialize:options;

import face_detector_hub;
import face_landmarker_hub;
import face_analyser;
import face_selector;
import face_masker_hub;

export namespace ffc::infra {

using namespace ffc::face_detector;
using namespace ffc::face_landmarker;
using namespace ffc::face_masker;
using json = nlohmann::json;

void to_json(json& j, cv::Size& v) {
    j = json{
        {"width", v.width},
        {"height", v.height},
    };
}

void from_json(json& j, cv::Size& v) {
    j.at("width").get_to(v.width);
    j.at("height").get_to(v.height);
}

NLOHMANN_JSON_SERIALIZE_ENUM(face_detector::FaceDetectorHub::Type,
                             {
                                 {face_detector::FaceDetectorHub::Type::Retina, "retinaface"},
                                 {face_detector::FaceDetectorHub::Type::Scrfd, "scrfd"},
                                 {face_detector::FaceDetectorHub::Type::Yolo, "yoloface"},
                             });

void to_json(json& j, std::unordered_set<FaceDetectorHub::Type>& v) {
    for (auto& type : v) {
        json j_type;
        to_json(j_type, type);
        j.push_back(j_type);
    }
}

void from_json(json& j, std::unordered_set<FaceDetectorHub::Type>& v) {
    for (auto& j_type : j) {
        FaceDetectorHub::Type type;
        from_json(j_type, type);
        v.insert(type);
    }
}

void to_json(json& j, FaceDetectorHub::Options& options) {
    json sz_json, types_json;
    to_json(sz_json, options.size);
    to_json(types_json, options.types);
    j = json{
        {"size", sz_json},
        {"models", types_json},
        {"angle", options.angle},
        {"min_score", options.min_score},
    };
}

void from_json(json& j, FaceDetectorHub::Options& options) {
    from_json(j.at("size"), options.size);
    from_json(j.at("models"), options.types);
    j.at("min_score").get_to(options.min_score);
}

NLOHMANN_JSON_SERIALIZE_ENUM(face_landmarker::FaceLandmarkerHub::Type,
                             {
                                 {face_landmarker::FaceLandmarkerHub::Type::_2DFAN, "2dfan4"},
                                 {face_landmarker::FaceLandmarkerHub::Type::PEPPA_WUTZ,
                                  "peppa_wutz"},
                             });

void to_json(json& j, std::unordered_set<FaceLandmarkerHub::Type>& v) {
    for (auto& type : v) {
        json j_type;
        to_json(j_type, type);
        j.push_back(j_type);
    }
}

void from_json(json& j, std::unordered_set<FaceLandmarkerHub::Type>& v) {
    for (auto& j_type : j) {
        FaceLandmarkerHub::Type type;
        from_json(j_type, type);
        v.insert(type);
    }
}

void to_json(json& j, FaceLandmarkerHub::Options& options) {
    json types_json;
    to_json(types_json, options.types);
    j = json{
        {"models", types_json},
        {"angle", options.angle},
        {"min_score", options.minScore},
    };
}

void from_json(json& j, FaceLandmarkerHub::Options& options) {
    from_json(j.at("models"), options.types);
    j.at("min_score").get_to(options.minScore);
}

NLOHMANN_JSON_SERIALIZE_ENUM(FaceSelector::FaceSelectorOrder,
                             {
                                 {FaceSelector::FaceSelectorOrder::Left_Right, "left_right"},
                                 {FaceSelector::FaceSelectorOrder::Right_Left, "right_left"},
                                 {FaceSelector::FaceSelectorOrder::Top_Bottom, "top_bottom"},
                                 {FaceSelector::FaceSelectorOrder::Bottom_Top, "bottom_top"},
                                 {FaceSelector::FaceSelectorOrder::Small_Large, "small_large"},
                                 {FaceSelector::FaceSelectorOrder::Large_Small, "large_small"},
                                 {FaceSelector::FaceSelectorOrder::Best_Worst, "best_worst"},
                                 {FaceSelector::FaceSelectorOrder::Worst_Best, "worst_best"},
                             });

NLOHMANN_JSON_SERIALIZE_ENUM(Gender, {
                                         {Gender::Male, "male"},
                                         {Gender::Female, "female"},
                                     });

NLOHMANN_JSON_SERIALIZE_ENUM(Race, {
                                       {Race::Black, "black"},
                                       {Race::Latino, "latino"},
                                       {Race::Indian, "indian"},
                                       {Race::Asian, "asian"},
                                       {Race::Arabic, "arabic"},
                                       {Race::White, "white"},
                                   });

void to_json(json& j, FaceSelector::Options& options) {
    j = json{
        {"order", options.order},         {"gender", options.genders},  {"race", options.races},
        {"age_start", options.age_start}, {"age_end", options.age_end},
    };
}

void from_json(json& j, FaceSelector::Options& options) {
    j.at("order").get_to(options.order);
    j.at("gender").get_to(options.genders);
    j.at("race").get_to(options.races);
    j.at("age_start").is_null() ? options.age_start :
                                  j.at("age_start").get_to<unsigned int>(options.age_start);
    j.at("age_end").is_null() ? options.age_end :
                                j.at("age_end").get_to<unsigned int>(options.age_end);
}

void to_json(nlohmann::json& j, FaceAnalyser::Options& options) {
    json fd_json, fl_json, fs_json;
    to_json(fd_json, options.faceDetectorOptions);
    to_json(fl_json, options.faceLandMarkerOptions);
    to_json(fs_json, options.faceSelectorOptions);
    j = json{
        {"face_detector", fd_json},
        {"face_landmarker", fl_json},
        {"face_selector", fs_json},
    };
}

void from_json(json& j, FaceAnalyser::Options& options) {
    from_json(j.at("face_detector"), options.faceDetectorOptions);
    from_json(j.at("face_landmarker"), options.faceLandMarkerOptions);
    from_json(j.at("face_selector"), options.faceSelectorOptions);
}

NLOHMANN_JSON_SERIALIZE_ENUM(face_masker::FaceMaskerHub::Type,
                             {
                                 {face_masker::FaceMaskerHub::Type::Box, "box"},
                                 {face_masker::FaceMaskerHub::Type::Occlusion, "occlusion"},
                                 {face_masker::FaceMaskerHub::Type::Region, "region"},
                             });

NLOHMANN_JSON_SERIALIZE_ENUM(
    face_masker::FaceMaskerRegion::Region,
    {
        {face_masker::FaceMaskerRegion::Region::Skin, "skin"},
        {face_masker::FaceMaskerRegion::Region::LeftEyebrow, "left_eyebrow"},
        {face_masker::FaceMaskerRegion::Region::RightEyebrow, "right_eyebrow"},
        {face_masker::FaceMaskerRegion::Region::LeftEye, "left_eye"},
        {face_masker::FaceMaskerRegion::Region::RightEye, "right_eye"},
        {face_masker::FaceMaskerRegion::Region::Glasses, "glasses"},
        {face_masker::FaceMaskerRegion::Region::Nose, "nose"},
        {face_masker::FaceMaskerRegion::Region::Mouth, "mouth"},
        {face_masker::FaceMaskerRegion::Region::UpperLip, "upper_lip"},
        {face_masker::FaceMaskerRegion::Region::LowerLip, "lower_lip"},
    });

void to_json(json& j, std::unordered_set<FaceMaskerRegion::Region>& regions) {
    j = json::array();
    for (auto region : regions) {
        json region_json;
        to_json(region_json, region);
        j.push_back(region_json);
    }
}

void from_json(json& j, std::unordered_set<FaceMaskerRegion::Region>& regions) {
    for (auto& region_json : j) {
        FaceMaskerRegion::Region region;
        from_json(region_json, region);
        regions.insert(region);
    }
}

} // namespace ffc::infra
