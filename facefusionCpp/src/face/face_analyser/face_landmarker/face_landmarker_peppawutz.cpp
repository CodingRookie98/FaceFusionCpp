/**
 ******************************************************************************
 * @file           : face_landmarker_peppawutz.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-26
 ******************************************************************************
 */

#include "face_landmarker_peppawutz.h"
#include "face_helper.h"
#include "platform.h"

FaceLandmarkerPeppawutz::FaceLandmarkerPeppawutz(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    FaceLandmarkerBase(env, modelPath) {
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][2];
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][3];
    if(m_inputHeight != m_inputSize.height || m_inputWidth != m_inputSize.width){
        throw std::runtime_error("Input size does not match the model");
    }
}

std::tuple<Face::Landmark, float> FaceLandmarkerPeppawutz::detect(const cv::Mat &visionFrame, const Face::BBox &bBox) {
    std::vector<float> inputData;
    cv::Mat invAffineMatrix;
    std::tie(inputData, invAffineMatrix) = preProcess(visionFrame, bBox);
    std::vector<int64_t> inputImgShape = {1, 3, m_inputHeight, m_inputWidth};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputData.data(), inputData.size(), inputImgShape.data(), inputImgShape.size());

    std::vector<Ort::Value> ortOutputs =
        m_inferenceSession.m_ortSession->Run(m_inferenceSession.m_runOptions,
                                             m_inferenceSession.m_inputNames.data(),
                                             &inputTensor, 1, m_inferenceSession.m_outputNames.data(),
                                             m_inferenceSession.m_outputNames.size());

    float *landmark68Data = ortOutputs[0].GetTensorMutableData<float>(); /// 形状是(1, 68, 3), 每一行的长度是3，表示一个关键点坐标x,y和置信度
    const int numPoints = ortOutputs[0].GetTensorTypeAndShapeInfo().GetShape()[1];
    std::vector<cv::Point2f> faceLandmark68(numPoints);
    std::vector<float> scores(numPoints);
    for (int i = 0; i < numPoints; i++) {
        float x = landmark68Data[i * 3] / 64.0 * (float)m_inputSize.width;
        float y = landmark68Data[i * 3 + 1] / 64.0 * (float)m_inputSize.width;
        float score = landmark68Data[i * 3 + 2];
        faceLandmark68[i] = cv::Point2f(x, y);
        scores[i] = score;
    }
    cv::transform(faceLandmark68, faceLandmark68, invAffineMatrix);

    float sum = 0.0;
    for (int i = 0; i < numPoints; i++) {
        sum += scores[i];
    }
    float meanScore = sum / (float)numPoints;
    meanScore = FaceHelper::interp({meanScore}, {0, 0.95}, {0, 1}).front();
    return std::make_tuple(Face::Landmark{faceLandmark68}, meanScore);
}

std::tuple<std::vector<float>, cv::Mat> FaceLandmarkerPeppawutz::preProcess(const cv::Mat &visionFrame, const Face::BBox &bBox) {
    float subMax = std::max(bBox.xmax - bBox.xmin, bBox.ymax - bBox.ymin);
    subMax = std::max(subMax, 1.f);
    const float scale = 195.f / subMax;
    const std::vector<float> translation = {((float)m_inputSize.width - (bBox.xmax + bBox.xmin) * scale) * 0.5f,
                                            ((float)m_inputSize.width - (bBox.ymax + bBox.ymin) * scale) * 0.5f};

    cv::Mat cropImg, affineMatrix;
    std::tie(cropImg, affineMatrix) = FaceHelper::warpFaceByTranslation(visionFrame, translation,
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
