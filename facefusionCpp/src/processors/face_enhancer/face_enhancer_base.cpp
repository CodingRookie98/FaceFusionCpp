/**
 ******************************************************************************
 * @file           : face_enhancer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

module face_enhancer;
import :face_enhancer_base;

namespace ffc::faceEnhancer {

cv::Mat FaceEnhancerBase::blendFrame(const cv::Mat &targetFrame, const cv::Mat &pasteVisionFrame, ushort blend) {
    if (blend > 100) {
        blend = 100;
    }
    const float faceEnhancerBlend = 1 - (static_cast<float>(blend) / 100.f);
    cv::Mat dstImage;
    cv::addWeighted(targetFrame, faceEnhancerBlend, pasteVisionFrame, 1 - faceEnhancerBlend, 0, dstImage);
    return dstImage;
}

} // namespace faceEnhancer