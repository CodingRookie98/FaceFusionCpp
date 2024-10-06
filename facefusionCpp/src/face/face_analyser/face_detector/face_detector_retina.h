/**
 ******************************************************************************
 * @file           : face_detector_retina.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_RETINA_H_
#define FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_RETINA_H_


#include "face_detector_base.h"

class FaceDetectorRetina : public FaceDetectorBase {
public:
    FaceDetectorRetina(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceDetectorRetina() override = default;
    
    Result detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                  const float &scoreThreshold) override;

private:
    std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize);
    int m_inputHeight{};
    int m_inputWidth{};
    const std::vector<int> m_featureStrides = {8, 16, 32};
    const int m_featureMapChannel = 3;
    const int m_anchorTotal = 2;
};

#endif // FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_RETINA_H_
