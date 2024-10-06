/**
 ******************************************************************************
 * @file           : face_masker_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_BASE_H_

#include <opencv2/opencv.hpp>
#include "inference_session.h"

class FaceMaskerBase {
public:
    FaceMaskerBase(const std::shared_ptr<Ort::Env> &env,
                   const std::string &modelPath);
    virtual ~FaceMaskerBase() = default;

    static cv::Mat createStaticBoxMask(const cv::Size &cropSize, const float &faceMaskBlur,
                                       const std::array<int, 4> &faceMaskPadding);

protected:
    Ffc::InferenceSession m_inferenceSession;
    Ort::RunOptions m_runOptions;
};
#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_BASE_H_
