/**
 ******************************************************************************
 * @file           : face_recognizer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#include "face_recognizer_base.h"

FaceRecognizerBase::FaceRecognizerBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    m_inferenceSession(env) {
    m_inferenceSession.createSession(modelPath);
}
