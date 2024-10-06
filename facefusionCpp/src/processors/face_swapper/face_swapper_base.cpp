/**
 ******************************************************************************
 * @file           : face_swapper_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "face_swapper_base.h"

FaceSwapperBase::FaceSwapperBase(const std::shared_ptr<Ort::Env> &env,
                                 const std::shared_ptr<FaceMaskers> &faceMaskers,
                                 const std::string &modelPath) :
    m_inferenceSession(env), m_faceMaskers(faceMaskers) {
    m_inferenceSession.createSession(modelPath);
}

std::string FaceSwapperBase::getProcessorName() {
    return "FaceSwapper";
}

ProcessorBase::ProcessorType FaceSwapperBase::getProcessorType() {
    return ProcessorBase::FaceSwapper;
}

void FaceSwapperBase::setMaskTypes(const std::unordered_set<FaceMaskers::Type> &maskerTypes) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    m_maskerTypes = maskerTypes;
}

void FaceSwapperBase::validateInputData(const ProcessorBase::InputData *inputData) {
    if (inputData == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData is nullptr."));
    }
    if (inputData->m_sourceFaces == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_sourceFaces is nullptr"));
    }
    if (inputData->m_targetFaces == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFaces is nullptr"));
    }
    if (inputData->m_targetFrame == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFrame is nullptr"));
    }
    
    if (inputData->m_sourceFaces->empty()) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_sourceFaces is empty"));
    }
    if (inputData->m_targetFrame->empty()) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFrame is empty"));
    }
}
