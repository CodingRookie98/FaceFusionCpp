/**
 ******************************************************************************
 * @file           : model_manager.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <fstream>
#include <unordered_set>
#include <nlohmann/json.hpp>

module model_manager;
import downloader;
import file_system;

namespace ffc {

std::unique_ptr<nlohmann::json> m_modelsInfoJson = nullptr;

ModelManager::ModelManager(const std::string &modelsInfoJsonPath) {
    m_modelsInfoJsonPath = modelsInfoJsonPath;
    if (std::ifstream file(m_modelsInfoJsonPath); file.is_open()) {
        // file >> m_modelsInfoJson;
        if (m_modelsInfoJson != nullptr) {
            m_modelsInfoJson->clear();
        }
        m_modelsInfoJson = std::make_unique<nlohmann::json>(nlohmann::json::parse(file));
        file.close();
    } else {
        throw std::runtime_error("Failed to open " + m_modelsInfoJsonPath);
    }
}

void *ModelManager::getModelInfo(const ModelManager::Model &model, const ModelManager::ModelInfoType &modelInfoType,
                                 const bool &skipDownload, std::string *errMsg) const {
    const std::string modelName = getModelName(model);
    const std::string modelPath = m_modelsInfoJson->at(modelName).at(m_modelInfoTypeMap.at(Path)).get<std::string>();
    const std::string modelUrl = m_modelsInfoJson->at(modelName).at(m_modelInfoTypeMap.at(Url)).get<std::string>();

    if (!skipDownload && !FileSystem::fileExists(modelPath)) {
        bool downloadSuccess = Downloader::download(modelUrl, "./models");
        if (!downloadSuccess) {
            if (errMsg) {
                *errMsg = "Failed to download the model file: " + modelPath;
            }
            throw std::runtime_error("Failed to download the model file: " + modelPath);
        }
    }

    const auto modelInfo = m_modelsInfoJson->at(modelName).at(m_modelInfoTypeMap.at(modelInfoType));
    if (modelInfo.is_null()) {
        if (errMsg) {
            *errMsg = "Model info not found";
        }
        return nullptr;
    }

    if (modelInfoType == ModelInfoType::Path) {
        if (modelPath.empty()) {
            if (errMsg) {
                *errMsg = "Model path is empty";
            }
            return nullptr;
        }
        return new std::string(modelPath);
    }
    if (modelInfoType == ModelInfoType::Url) {
        if (modelUrl.empty()) {
            if (errMsg) {
                *errMsg = "Model url is empty";
            }
            return nullptr;
        }
        return new std::string(modelUrl);
    }
    if (errMsg) {
        *errMsg = "Unknown model info modelInfoType";
    }
    return nullptr;
}

std::string ModelManager::getModelUrl(const ModelManager::Model &model, std::string *errMsg) const {
    std::string modelUrl = m_modelsInfoJson->at(getModelName(model)).at(m_modelInfoTypeMap.at(Url)).get<std::string>();
    if (modelUrl.empty()) {
        if (errMsg) {
            *errMsg = "Model url is empty";
        }
        throw std::runtime_error("Model url is empty");
    }
    return modelUrl;
}

std::string ModelManager::getModelPath(const ModelManager::Model &model, const bool &skipDownload, std::string *errMsg) const {
    std::string modelName = getModelName(model);
    std::string modelPath = m_modelsInfoJson->at(modelName).at(m_modelInfoTypeMap.at(Path)).get<std::string>();
    modelPath = FileSystem::absolutePath(modelPath);
    std::string modelUrl = getModelUrl(model);

    if (!skipDownload && !FileSystem::fileExists(modelPath)) {
        bool downloadSuccess = Downloader::download(modelUrl, "./models");
        if (!downloadSuccess) {
            if (errMsg) {
                *errMsg = "Failed to download the model file: " + modelPath;
            }
            throw std::runtime_error("Failed to download the model file: " + modelPath);
        }
    }

    return modelPath;
}

std::unordered_set<std::string> ModelManager::getModelsUrl() const {
    std::unordered_set<std::string> modelUrls;
    for (const auto &model : *m_modelsInfoJson) {
        if (!model.is_object()) {
            continue;
        }
        modelUrls.insert(model.at("url").get<std::string>());
    }
    return modelUrls;
}

std::string ModelManager::getModelName(const ModelManager::Model &model) {
    static std::unordered_map<Model, std::string> modelNames = {
        {Model::Gfpgan_12, "gfpgan_1.2"},
        {Model::Gfpgan_13, "gfpgan_1.3"},
        {Model::Gfpgan_14, "gfpgan_1.4"},
        {Model::Codeformer, "codeformer"},
        {Model::Inswapper_128, "inswapper_128"},
        {Model::Inswapper_128_fp16, "inswapper_128_fp16"},
        {Model::Face_detector_retinaface, "face_detector_retinaface"},
        {Model::Face_detector_scrfd, "face_detector_scrfd"},
        {Model::Face_detector_yoloface, "face_detector_yoloface"},
        {Model::Face_recognizer_arcface_w600k_r50, "face_recognizer_arcface_w600k_r50"},
        {Model::Face_landmarker_68, "face_landmarker_68"},
        {Model::Face_landmarker_peppawutz, "face_landmarker_peppa_wutz"},
        {Model::Face_landmarker_68_5, "face_landmarker_68_5"},
        {Model::FairFace, "fairface"},
        {Model::bisenet_resnet_18, "bisenet_resnet_18"},
        {Model::bisenet_resnet_34, "bisenet_resnet_34"},
        {Model::xseg_1, "xseg_1"},
        {Model::xseg_2, "xseg_2"},
        {Model::Feature_extractor, "feature_extractor"},
        {Model::Motion_extractor, "motion_extractor"},
        {Model::Generator, "generator"},
        {Model::Real_esrgan_x2, "real_esrgan_x2"},
        {Model::Real_esrgan_x2_fp16, "real_esrgan_x2_fp16"},
        {Model::Real_esrgan_x4, "real_esrgan_x4"},
        {Model::Real_esrgan_x4_fp16, "real_esrgan_x4_fp16"},
        {Model::Real_esrgan_x8, "real_esrgan_x8"},
        {Model::Real_esrgan_x8_fp16, "real_esrgan_x8_fp16"},
        {Model::Real_hatgan_x4, "real_hatgan_x4"},
    };
    return modelNames[model];
}

std::shared_ptr<ModelManager> ModelManager::getInstance(const std::string &modelsInfoJsonPath) {
    static std::shared_ptr<ModelManager> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<ModelManager>(modelsInfoJsonPath); });
    return instance;
}

bool ModelManager::downloadAllModel() const {
    std::unordered_set<std::string> modelUrls = getModelsUrl();
    return ffc::Downloader::batchDownload(modelUrls, "./models");
}

} // namespace ffc