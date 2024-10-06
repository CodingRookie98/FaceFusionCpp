/**
 ******************************************************************************
 * @file           : face_masker_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#include "face_masker_base.h"
#include "platform.h" // don't remove this line

FaceMaskerBase::FaceMaskerBase(const std::shared_ptr<Ort::Env> &env,
                               const std::string &modelPath) :
    m_inferenceSession(env) {
    m_inferenceSession.createSession(modelPath);
}

cv::Mat FaceMaskerBase::createStaticBoxMask(const cv::Size &cropSize, const float &faceMaskBlur, const std::array<int, 4> &faceMaskPadding) {
    int blurAmount = static_cast<int>(cropSize.width * 0.5 * faceMaskBlur);
    int blurArea = std::max(blurAmount / 2, 1);

    cv::Mat boxMask(cropSize, CV_32F, cv::Scalar(1.0f));

    int paddingTop = std::max(blurArea, static_cast<int>(cropSize.height * faceMaskPadding.at(0) / 100.0));
    int paddingBottom = std::max(blurArea, static_cast<int>(cropSize.height * faceMaskPadding.at(2) / 100.0));
    int paddingLeft = std::max(blurArea, static_cast<int>(cropSize.width * faceMaskPadding.at(3) / 100.0));
    int paddingRight = std::max(blurArea, static_cast<int>(cropSize.width * faceMaskPadding.at(1) / 100.0));

    boxMask(cv::Range(0, paddingTop), cv::Range::all()) = 0;
    boxMask(cv::Range(cropSize.height - paddingBottom, cropSize.height), cv::Range::all()) = 0;
    boxMask(cv::Range::all(), cv::Range(0, paddingLeft)) = 0;
    boxMask(cv::Range::all(), cv::Range(cropSize.width - paddingRight, cropSize.width)) = 0;

    if (blurAmount > 0) {
        cv::GaussianBlur(boxMask, boxMask, cv::Size(0, 0), blurAmount * 0.25);
    }

    return boxMask;
}