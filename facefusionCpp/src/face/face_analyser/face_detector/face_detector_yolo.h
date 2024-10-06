/**
 ******************************************************************************
 * @file           : face_yolov_8.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-4
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_YOLO_H_
#define FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_YOLO_H_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define NOMINMAX
#endif

#include <opencv2/opencv.hpp>
#include "face_detector_base.h"

class FaceDetectorYolo : public FaceDetectorBase {
public:
    explicit FaceDetectorYolo(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceDetectorYolo() override = default;

    Result detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize, const float &scoreThreshold);

private:
    std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize);
    int m_inputHeight;
    int m_inputWidth;
};

#endif // FACEFUSIONCPP_SRC_FACE_ANALYSER_FACE_DETECTOR_YOLO_H_
