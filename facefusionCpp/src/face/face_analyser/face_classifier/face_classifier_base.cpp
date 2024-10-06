/**
 ******************************************************************************
 * @file           : face_classifier_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

#include "face_classifier_base.h"

FaceClassifierBase::FaceClassifierBase(const std::shared_ptr<Ort::Env> &env,
                                       const std::string &modelPath):
    m_inferenceSession(env){
    m_inferenceSession.createSession(modelPath);
}
