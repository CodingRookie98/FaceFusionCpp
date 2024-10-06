/**
 ******************************************************************************
 * @file           : face_masker_occlusion.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_OCCLUSION_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_OCCLUSION_H_

#include "face_masker_base.h"

class FaceMaskerOcclusion : public FaceMaskerBase {
public:
    FaceMaskerOcclusion(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceMaskerOcclusion() override = default;

    cv::Mat createOcclusionMask(const cv::Mat &cropVisionFrame);

private:
    int m_inputHeight;
    int m_inputWidth;
    
    std::vector<float> getInputImageData(const cv::Mat &cropVisionFrame) const;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_OCCLUSION_H_
