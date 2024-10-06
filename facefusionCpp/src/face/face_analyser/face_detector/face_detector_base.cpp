/**
 ******************************************************************************
 * @file           : face_detector_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

#include "face_detector_base.h"
#include "face_helper.h"

FaceDetectorBase::FaceDetectorBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    m_inferenceSession(env) {
    m_inferenceSession.createSession(modelPath);
}

FaceDetectorBase::Result
FaceDetectorBase::detectRotatedFaces(const cv::Mat &visionFrame,
                                     const cv::Size &faceDetectorSize,
                                     const double &angle, const float &detectorScore) {
    cv::Mat rotatedMat, rotatedVisionFrame, rotatedInverseMat;
    cv::Size rotatedSize;
    std::tie(rotatedMat, rotatedSize) = FaceHelper::createRotatedMatAndSize(angle, visionFrame.size());
    cv::warpAffine(visionFrame, rotatedVisionFrame, rotatedMat, rotatedSize);
    cv::invertAffineTransform(rotatedMat, rotatedInverseMat);
    Result result = detectFaces(rotatedVisionFrame, faceDetectorSize, detectorScore);
    for (auto &bbox : result.bboxes) {
       bbox  = FaceHelper::transformBBox(bbox, rotatedInverseMat);
    }

    for (auto &landmarkVec : result.landmarks) {
        landmarkVec = FaceHelper::transformPoints(landmarkVec, rotatedInverseMat);
    }

    return result;
}
