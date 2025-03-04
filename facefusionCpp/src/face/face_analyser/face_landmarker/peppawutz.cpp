/**
 ******************************************************************************
 * @file           : face_landmarker_peppawutz.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-26
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include "common_macros.h"

module face_landmarker_hub;
import :peppawutz;
import face_helper;

namespace ffc::faceLandmarker {
Peppawutz::Peppawutz(const std::shared_ptr<Ort::Env> &env) :
    FaceLandmarkerBase(env) {
}

void Peppawutz::LoadModel(const std::string &modelPath, const Options &options) {
    FaceLandmarkerBase::LoadModel(modelPath, options);
    m_inputHeight = input_node_dims_[0][2];
    m_inputWidth = input_node_dims_[0][3];
    m_inputSize = cv::Size(m_inputWidth, m_inputHeight);
}

std::tuple<Face::Landmarks, float> Peppawutz::detect(const cv::Mat &visionFrame, const Face::BBox &bBox) const {
    auto [inputData, invAffineMatrix] = preProcess(visionFrame, bBox);
    std::vector<int64_t> inputImgShape = {1, 3, m_inputHeight, m_inputWidth};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memory_info_->GetConst(), inputData.data(), inputData.size(), inputImgShape.data(), inputImgShape.size());

    std::vector<Ort::Value> ortOutputs = ort_session_->Run(run_options_, input_names_.data(),
                                                           &inputTensor, 1, output_names_.data(),
                                                           output_names_.size());

    float *landmark68Data = ortOutputs[0].GetTensorMutableData<float>(); /// 形状是(1, 68, 3), 每一行的长度是3，表示一个关键点坐标x,y和置信度
    const int numPoints = ortOutputs[0].GetTensorTypeAndShapeInfo().GetShape()[1];
    std::vector<cv::Point2f> faceLandmark68(numPoints);
    std::vector<float> scores(numPoints);
    for (int i = 0; i < numPoints; i++) {
        const float x = landmark68Data[i * 3] / 64.0 * static_cast<float>(m_inputSize.width);
        const float y = landmark68Data[i * 3 + 1] / 64.0 * static_cast<float>(m_inputSize.width);
        const float score = landmark68Data[i * 3 + 2];
        faceLandmark68[i] = cv::Point2f(x, y);
        scores[i] = score;
    }
    cv::transform(faceLandmark68, faceLandmark68, invAffineMatrix);

    float sum = 0.0;
    for (int i = 0; i < numPoints; i++) {
        sum += scores[i];
    }
    float meanScore = sum / static_cast<float>(numPoints);
    meanScore = face_helper::interp({meanScore}, {0, 0.95}, {0, 1}).front();
    return std::make_tuple(Face::Landmarks{faceLandmark68}, meanScore);
}

std::tuple<std::vector<float>, cv::Mat> Peppawutz::preProcess(const cv::Mat &visionFrame, const Face::BBox &bBox) const {
    float subMax = std::max(bBox.xMax - bBox.xMin, bBox.yMax - bBox.yMin);
    subMax = std::max(subMax, 1.f);
    const float scale = 195.f / subMax;
    const std::vector<float> translation = {(static_cast<float>(m_inputSize.width) - (bBox.xMax + bBox.xMin) * scale) * 0.5f,
                                            (static_cast<float>(m_inputSize.width) - (bBox.yMax + bBox.yMin) * scale) * 0.5f};

    auto [cropImg, affineMatrix] = face_helper::warpFaceByTranslation(visionFrame, translation,
                                                                     scale, m_inputSize);
    cropImg = conditionalOptimizeContrast(cropImg);
    cv::Mat invAffineMatrix;
    cv::invertAffineTransform(affineMatrix, invAffineMatrix);

    std::vector<cv::Mat> bgrChannels(3);
    split(cropImg, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / 255.0);
    }

    const int imageArea = this->m_inputHeight * this->m_inputWidth;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float *)bgrChannels[0].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float *)bgrChannels[2].data, singleChnSize);

    return std::make_tuple(inputData, invAffineMatrix);
}
}