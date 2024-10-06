/**
 ******************************************************************************
 * @file           : face_landmarker_68_5.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_LANDMARKER_68_5_H_
#define FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_LANDMARKER_68_5_H_

#include "face.h"
#include "face_landmarker_base.h"

class FaceLandmarker68By5 : public FaceLandmarkerBase {
public:
    explicit FaceLandmarker68By5(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceLandmarker68By5() override = default;

    Face::Landmark detect(const Face::Landmark &faceLandmark5);

private:
    std::tuple<std::vector<float>, cv::Mat> preProcess(const Face::Landmark &faceLandmark5);
    int m_inputHeight;
    int m_inputWidth;
};

#endif // FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_LANDMARKER_68_5_H_
