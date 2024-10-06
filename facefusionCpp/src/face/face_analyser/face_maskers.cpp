/**
 ******************************************************************************
 * @file           : face_maskers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "face_maskers.h"
#include "face_masker_occlusion.h"
#include "model_manager.h"

FaceMaskers::FaceMaskers(const std::shared_ptr<Ort::Env> &env) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceMaskers");
    } else {
        m_env = env;
    }

    setFaceMaskPadding();
    setFaceMaskBlur();
}

FaceMaskers::~FaceMaskers() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto masker : m_maskers) {
        delete masker.second;
    }
    m_maskers.clear();
}

void FaceMaskers::createMasker(const FaceMaskers::Type &type) {
    if (m_maskers.contains(type)) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (m_maskers[type] != nullptr) {
            return;
        }
    }

    static std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    FaceMaskerBase *masker = nullptr;
    if (type == Region) {
        masker = new FaceMaskerRegion(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_parser));
    }
    if (type == Occlusion) {
        masker = new FaceMaskerOcclusion(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_occluder));
    }
    if (masker != nullptr) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_maskers[type] = masker;
    }
}

cv::Mat FaceMaskers::createOcclusionMask(const cv::Mat &cropVisionFrame) {
    createMasker(Occlusion);
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    FaceMaskerOcclusion *faceMaskerOcclusion = dynamic_cast<FaceMaskerOcclusion *>(m_maskers[Occlusion]);
    lock.unlock();
    return faceMaskerOcclusion->createOcclusionMask(cropVisionFrame);
}

cv::Mat FaceMaskers::createRegionMask(const cv::Mat &inputImage,
                                      const std::unordered_set<FaceMaskerRegion::Region> &regions) {
    createMasker(Region);

    std::shared_lock<std::shared_mutex> lock(m_mutex);
    FaceMaskerRegion *faceMaskerRegion = dynamic_cast<FaceMaskerRegion *>(m_maskers[Region]);
    lock.unlock();

    return faceMaskerRegion->createRegionMask(inputImage, regions);
}

cv::Mat FaceMaskers::createRegionMask(const cv::Mat &inputImage) {
    if (m_regions.empty()) {
        throw std::runtime_error("Before using the function createRegionMask(const cv::Mat &inputImage), please call the function FaceMaskers::setFaceMaskRegions(const std::unordered_set<FaceMaskerRegion> &regions).");
    }
    return createRegionMask(inputImage, m_regions);
}

cv::Mat FaceMaskers::createStaticBoxMask(const cv::Size &cropSize, const float &faceMaskBlur, const std::array<int, 4> &faceMaskPadding) {
    return FaceMaskerBase::createStaticBoxMask(cropSize, faceMaskBlur, faceMaskPadding);
}

cv::Mat FaceMaskers::createStaticBoxMask(const cv::Size &cropSize) {
    return createStaticBoxMask(cropSize, m_faceMaskBlur, m_padding);
}

void FaceMaskers::setFaceMaskPadding(const std::array<int, 4> &padding) {
    m_padding = padding;
}

void FaceMaskers::setFaceMaskBlur(const float &faceMaskBlur) {
    m_faceMaskBlur = faceMaskBlur;
}

void FaceMaskers::setFaceMaskRegions(const std::unordered_set<FaceMaskerRegion::Region> &regions) {
    m_regions = regions;
}

cv::Mat FaceMaskers::getBestMask(const std::vector<cv::Mat> &masks) {
    if (masks.empty()) {
        throw std::invalid_argument("The input vector is empty.");
    }

    // Initialize the minMask with the first mask in the vector
    cv::Mat minMask = masks[0].clone();

    for (size_t i = 1; i < masks.size(); ++i) {
        // Check if the current mask has the same size and type as minMask
        if (masks[i].size() != minMask.size() || masks[i].type() != minMask.type()) {
            throw std::invalid_argument("[FaceMasker] All masks must have the same size and type.");
        }
        cv::min(minMask, masks[i], minMask);
    }
    minMask.setTo(0, minMask < 0);
    minMask.setTo(1, minMask > 1);

    return minMask;
}
