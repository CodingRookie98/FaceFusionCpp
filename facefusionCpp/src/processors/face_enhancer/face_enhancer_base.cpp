/**
 ******************************************************************************
 * @file           : face_enhancer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

#include "face_enhancer_base.h"
void FaceEnhancerBase::setMaskTypes(const std::unordered_set<FaceMaskers::Type> &maskerTypes) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    m_maskerTypes = maskerTypes;
    m_faceBlend = 80;
}

ProcessorBase::ProcessorType FaceEnhancerBase::getProcessorType() {
    return ProcessorBase::ProcessorType::FaceEnhancer;
}

std::string FaceEnhancerBase::getProcessorName() {
    return "FaceEnhancer";
}

FaceEnhancerBase::FaceEnhancerBase(const std::shared_ptr<Ort::Env> &env, const std::shared_ptr<FaceMaskers> &faceMaskers, const std::string &modelPath) :
    m_inferenceSession(env), m_faceMaskers(faceMaskers) {
    m_inferenceSession.createSession(modelPath);
}

void FaceEnhancerBase::setFaceBlend(const unsigned int blend) {
    if (blend > 100) {
        m_faceBlend = 100;
    } else if (blend <= 0) {
        m_faceBlend = 0;
    }
}

cv::Mat FaceEnhancerBase::blendFrame(const cv::Mat &targetFrame, const cv::Mat &pasteVisionFrame) const {
    const float faceEnhancerBlend = 1 - ((float)m_faceBlend / 100.f);
    cv::Mat dstImage;
    cv::addWeighted(targetFrame, faceEnhancerBlend, pasteVisionFrame, 1 - faceEnhancerBlend, 0, dstImage);
    return dstImage;
}

void FaceEnhancerBase::validateInputData(const ProcessorBase::InputData *inputData) {
    if (inputData == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->inputData is nullptr."));
    }
    if (inputData->m_targetFrame == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->targetFrame is nullptr."));
    }
    if (inputData->m_targetFaces == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->targetFaces is nullptr."));
    }
    
    if (inputData->m_targetFrame->empty()) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->targetFrame is empty."));
    }
}
