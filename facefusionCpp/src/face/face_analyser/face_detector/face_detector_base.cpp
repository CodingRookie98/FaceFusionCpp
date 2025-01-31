/**
 ******************************************************************************
 * @file           : face_detector_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_detector_hub;
import :face_detector_base;
import face_helper;

namespace ffc::faceDetector {
FaceDetectorBase::FaceDetectorBase(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}

FaceDetectorBase::Result
FaceDetectorBase::detectRotatedFaces(const cv::Mat &visionFrame,
                                     const cv::Size &faceDetectorSize,
                                     const double &angle, const float &detectorScore) {
    cv::Mat rotatedVisionFrame, rotatedInverseMat;
    auto [rotatedMat, rotatedSize] = FaceHelper::createRotatedMatAndSize(angle, visionFrame.size());
    cv::warpAffine(visionFrame, rotatedVisionFrame, rotatedMat, rotatedSize);
    cv::invertAffineTransform(rotatedMat, rotatedInverseMat);
    Result result = detectFaces(rotatedVisionFrame, faceDetectorSize, detectorScore);
    for (auto &bbox : result.bboxes) {
        bbox = FaceHelper::transformBBox(bbox, rotatedInverseMat);
    }

    for (auto &landmarkVec : result.landmarks) {
        landmarkVec = FaceHelper::transformPoints(landmarkVec, rotatedInverseMat);
    }

    return result;
}
} // namespace ffc::faceDetector