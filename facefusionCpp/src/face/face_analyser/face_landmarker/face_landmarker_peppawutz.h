/**
 ******************************************************************************
 * @file           : face_landmarker_peppawutz.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-26
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_PEPPAWUTZ_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_PEPPAWUTZ_H_

#include "face_landmarker_base.h"
#include "face.h"

class FaceLandmarkerPeppawutz : public FaceLandmarkerBase {
public:
    FaceLandmarkerPeppawutz(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceLandmarkerPeppawutz() override = default;

    std::tuple<Face::Landmark, float> detect(const cv::Mat &visionFrame, const Face::BBox &bBox);

private:
    int m_inputHeight;
    int m_inputWidth;
    cv::Size m_inputSize{256, 256};
    std::tuple<std::vector<float>, cv::Mat> preProcess(const cv::Mat &visionFrame, const Face::BBox &bBox);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_PEPPAWUTZ_H_
