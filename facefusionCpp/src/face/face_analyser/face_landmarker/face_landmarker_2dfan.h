/**
 ******************************************************************************
 * @file           : face_landmarker_2_dfan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_2DFAN_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_2DFAN_H_

#include "face.h"
#include "face_landmarker_base.h"

class FaceLandmarker2dfan : public FaceLandmarkerBase {
public:
    FaceLandmarker2dfan(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceLandmarker2dfan() override = default;

    // Return the coordinates and confidence values for the 68 facial landmarks
    // 1 is landmark, 2 is confidence
    std::tuple<Face::Landmark, float> detect(const cv::Mat &visionFrame, const Face::BBox &bBox);

private:
    int m_inputHeight;
    int m_inputWidth;
    cv::Size m_inputSize{256, 256};
    std::tuple<std::vector<float>, cv::Mat> preProcess(const cv::Mat &visionFrame, const Face::BBox &bBox);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_2DFAN_H_
