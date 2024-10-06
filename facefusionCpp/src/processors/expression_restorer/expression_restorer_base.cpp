/**
 ******************************************************************************
 * @file           : exprssion_restorer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#include "expression_restorer_base.h"

ExpressionRestorerBase::ExpressionRestorerBase(const std::shared_ptr<Ort::Env> &env,
                                               const std::shared_ptr<FaceMaskers> &faceMaskers,
                                               const std::string &featureExtractorPath,
                                               const std::string &motionExtractorPath,
                                               const std::string &generatorPath) :
    m_featureSession(env), m_motionSession(env), m_generatorSession(env) {
    m_faceMaskers = faceMaskers;
    m_featureSession.createSession(featureExtractorPath);
    m_motionSession.createSession(motionExtractorPath);
    m_generatorSession.createSession(generatorPath);
}

std::string ExpressionRestorerBase::getProcessorName() {
    return "ExpressionRestorer";
}

ProcessorBase::ProcessorType ExpressionRestorerBase::getProcessorType() {
    return ProcessorBase::ExpressionRestorer;
}

void ExpressionRestorerBase::setMaskTypes(const std::unordered_set<FaceMaskers::Type> &maskerTypes) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    m_maskerTypes = maskerTypes;
}

void ExpressionRestorerBase::validateInputData(const ProcessorBase::InputData *inputData) {
    if (inputData == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData is nullptr."));
    }
    if (inputData->m_originalTargetFrame == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_originalTargetFrame is nullptr."));
    }
    if (inputData->m_targetFrame == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFrame is nullptr."));
    }
    if (inputData->m_targetFaces == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFaces is nullptr."));
    }
    
    if (inputData->m_originalTargetFrame->empty()) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_originalTargetFrame is empty."));
    }
    if (inputData->m_targetFrame->empty()) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFrame is empty."));
    }
}
