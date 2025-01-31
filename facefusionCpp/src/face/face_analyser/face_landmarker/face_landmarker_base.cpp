/**
 ******************************************************************************
 * @file           : face_landmarker_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_landmarker_hub;
import :face_landmarker_base;

namespace ffc::faceLandmarker {
FaceLandmarkerBase::FaceLandmarkerBase(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}

cv::Mat FaceLandmarkerBase::conditionalOptimizeContrast(const cv::Mat &visionFrame) {
    cv::Mat result;
    cv::cvtColor(visionFrame, result, cv::COLOR_BGR2Lab);

    // Only process if mean L value is less than 30.0
    cv::Scalar mean = cv::mean(result.reshape(1, 0));
    if (mean[0] < 30.0) {
        std::vector<cv::Mat> labChannels;
        cv::split(result, labChannels);

        // Apply CLAHE only to the L channel
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(labChannels[0], labChannels[0]);

        cv::merge(labChannels, result);
    }

    cv::cvtColor(result, result, cv::COLOR_Lab2BGR);
    return result;
}
} // namespace ffc::faceLandmarker