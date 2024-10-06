/**
 ******************************************************************************
 * @file           : face_masker_occlusion.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

#include "face_masker_occlusion.h"

FaceMaskerOcclusion::FaceMaskerOcclusion(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    FaceMaskerBase(env, modelPath) {
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][1];
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][2];
}

cv::Mat FaceMaskerOcclusion::createOcclusionMask(const cv::Mat &cropVisionFrame) {
    std::vector<float> inputImageData = getInputImageData(cropVisionFrame);

    std::vector<int64_t> inputImageShape{1, m_inputHeight, m_inputWidth, 3};
    std::vector<Ort::Value> inputTensors;
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputImageData.data(),
                                                              inputImageData.size(),
                                                              inputImageShape.data(),
                                                              inputImageShape.size()));

    std::vector<Ort::Value> outputTensors = m_inferenceSession.m_ortSession->Run(m_runOptions, m_inferenceSession.m_inputNames.data(),
                                                                                 inputTensors.data(), inputTensors.size(),
                                                                                 m_inferenceSession.m_outputNames.data(), m_inferenceSession.m_outputNames.size());

    float *pdata = outputTensors[0].GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int outputHeight = outsShape[1];
    const int outputWidth = outsShape[2];

    cv::Mat mask(outputHeight, outputWidth, CV_32FC1, pdata);
    mask.setTo(0, mask < 0);
    mask.setTo(1, mask > 1);

    cv::resize(mask, mask, cropVisionFrame.size());
    cv::GaussianBlur(mask, mask, cv::Size(0, 0), 5);
    mask.setTo(0.5, mask < 0.5);
    mask.setTo(1, mask > 1);
    mask = (mask - 0.5) * 2;
    
    return mask;
}

std::vector<float> FaceMaskerOcclusion::getInputImageData(const cv::Mat &cropVisionFrame) const {
    cv::Mat inputImage;
    cv::resize(cropVisionFrame, inputImage, cv::Size(m_inputWidth, m_inputHeight));
    std::vector<cv::Mat> bgrChannels;
    cv::split(inputImage, bgrChannels);
    for (int i = 0; i < 3; i++) {
        bgrChannels[i].convertTo(bgrChannels[i], CV_32FC1, 1.0 / 255.0);
    }
    const int imageArea = inputImage.rows * inputImage.cols;
    std::vector<float> inputImageData(3 * imageArea);

    // 妈的，傻逼输入形状(1, 256, 256, 3), 害得老子排查了一天半的bug, 老子服了
    int k = 0;
    for (int i = 0; i < m_inputHeight; ++i) {
        for (int j = 0; j < m_inputWidth; ++j) {
            inputImageData.at(k) = bgrChannels[2].at<float>(i, j);
            inputImageData.at(k + 1) = bgrChannels[1].at<float>(i, j);
            inputImageData.at(k + 2) = bgrChannels[0].at<float>(i, j);
            k += 3;
        }
    }
    return inputImageData;
}
