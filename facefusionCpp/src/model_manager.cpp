/**
 ******************************************************************************
 * @file           : model_manager.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

#include "model_manager.h"
#include <fstream>
#include "downloader.h"
#include "file_system.h"

namespace Ffc {
ModelManager::ModelManager(const std::string &modelsInfoJsonPath) {
    m_modelsInfoJsonPath = modelsInfoJsonPath;
    std::ifstream file(m_modelsInfoJsonPath);
    if (file.is_open()) {
        file >> m_modelsInfoJson;
        file.close();
    } else {
        throw std::runtime_error("Failed to open " + m_modelsInfoJsonPath);
    }
}

void *ModelManager::getModelInfo(const ModelManager::Model &model, const ModelManager::ModelInfoType &modelInfoType,
                                 const bool &skipDownload, std::string *errMsg) const {
    std::string modelName = getModelName(model);
    std::string modelPath = m_modelsInfoJson.at(modelName).at(m_modelInfoTypeMap.at(Path)).get<std::string>();
    std::string modelUrl = m_modelsInfoJson.at(modelName).at(m_modelInfoTypeMap.at(Url)).get<std::string>();

    if (!skipDownload && !FileSystem::fileExists(modelPath)) {
        bool downloadSuccess = Downloader::download(modelUrl, "./models");
        if (!downloadSuccess) {
            if (errMsg) {
                *errMsg = "Failed to download the model file: " + modelPath;
            }
            throw std::runtime_error("Failed to download the model file: " + modelPath);
        }
    }

    auto modelInfo = m_modelsInfoJson.at(modelName).at(m_modelInfoTypeMap.at(modelInfoType));
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
    } else if (modelInfoType == ModelInfoType::Url) {
        if (modelUrl.empty()) {
            if (errMsg) {
                *errMsg = "Model url is empty";
            }
            return nullptr;
        }
        return new std::string(modelUrl);
    } else {
        if (errMsg) {
            *errMsg = "Unknown model info modelInfoType";
        }
        return nullptr;
    }
}

std::string ModelManager::getModelUrl(const ModelManager::Model &model, std::string *errMsg) const {
    std::string modelUrl = m_modelsInfoJson.at(getModelName(model)).at(m_modelInfoTypeMap.at(Url)).get<std::string>();
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
    std::string modelPath = m_modelsInfoJson.at(modelName).at(m_modelInfoTypeMap.at(Path)).get<std::string>();
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
    for (const auto &model : m_modelsInfoJson) {
        if (!model.is_object()) {
            continue;
        }
        modelUrls.insert(model.at("url").get<std::string>());
    }
    return modelUrls;
}

std::string ModelManager::getModelName(const ModelManager::Model &model) const {
    static std::unordered_map<Model, std::string> modelNames = {
        {Gfpgan_12, "gfpgan_1.2"},
        {Gfpgan_13, "gfpgan_1.3"},
        {Gfpgan_14, "gfpgan_1.4"},
        {Codeformer, "codeformer"},
        {Inswapper_128, "inswapper_128"},
        {Inswapper_128_fp16, "inswapper_128_fp16"},
        {Face_detector_retinaface, "face_detector_retinaface"},
        {Face_detector_scrfd, "face_detector_scrfd"},
        {Face_detector_yoloface, "face_detector_yoloface"},
        {Face_recognizer_arcface_w600k_r50, "face_recognizer_arcface_w600k_r50"},
        {Face_landmarker_68, "face_landmarker_68"},
        {Face_landmarker_peppawutz, "face_landmarker_peppa_wutz"},
        {Face_landmarker_68_5, "face_landmarker_68_5"},
        {FairFace, "fairface"},
        {Face_parser, "face_parser"},
        {Face_occluder, "face_occluder"},
        {Feature_extractor, "feature_extractor"},
        {Motion_extractor, "motion_extractor"},
        {Generator, "generator"},
        {Real_esrgan_x2, "real_esrgan_x2"},
        {Real_esrgan_x2_fp16, "real_esrgan_x2_fp16"},
        {Real_esrgan_x4, "real_esrgan_x4"},
        {Real_esrgan_x4_fp16, "real_esrgan_x4_fp16"},
        {Real_esrgan_x8, "real_esrgan_x8"},
        {Real_esrgan_x8_fp16, "real_esrgan_x8_fp16"},
        {Real_hatgan_x4, "real_hatgan_x4"},
    };
    return modelNames[model];
}

std::shared_ptr<ModelManager> ModelManager::getInstance(const std::string &modelsInfoJsonPath) {
    static std::shared_ptr<ModelManager> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<ModelManager>(modelsInfoJsonPath); });
    return instance;
}

bool ModelManager::downloadAllModel() {
    std::unordered_set<std::string> modelUrls = getModelsUrl();
    return Ffc::Downloader::batchDownload(modelUrls, "./models");
}

} // namespace Ffc