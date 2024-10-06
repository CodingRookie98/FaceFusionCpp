/**
 ******************************************************************************
 * @file           : fr_arc_w_600_k_r_50.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FR_ARC_W_600_K_R_50_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FR_ARC_W_600_K_R_50_H_

#include "face_recognizer_base.h"

class FRArcW600kR50 : public FaceRecognizerBase {
public:
    // model: arcface_w600k_r50.onnx
    FRArcW600kR50(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FRArcW600kR50() override = default;

    // return: [0] embedding, [1] normedEmbedding
    std::array<std::vector<float>, 2> recognize(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5) override;

private:
    std::vector<float> preProcess(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5_68);
    int m_inputWidth;
    int m_inputHeight;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FR_ARC_W_600_K_R_50_H_
