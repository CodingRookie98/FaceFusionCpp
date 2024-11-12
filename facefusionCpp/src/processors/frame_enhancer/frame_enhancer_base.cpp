/**
 ******************************************************************************
 * @file           : frame_enhancer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-30
 ******************************************************************************
 */

#include "frame_enhancer_base.h"
FrameEnhancerBase::FrameEnhancerBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    m_inferenceSession(env) {
    m_inferenceSession.createSession(modelPath);
}

std::string FrameEnhancerBase::getProcessorName() {
    return "FrameEnhancer";
}

ProcessorBase::ProcessorType FrameEnhancerBase::getProcessorType() {
    return ProcessorBase::FrameEnhancer;
}

cv::Mat FrameEnhancerBase::blendFrame(const cv::Mat &tempFrame, const cv::Mat &mergedFrame) const {
    float blendFactor = 1 - ((float)m_blend / 100.f);
    cv::Mat result;
    cv::resize(tempFrame, result, mergedFrame.size());
    cv::addWeighted(result, blendFactor, mergedFrame, 1 - blendFactor, 0, result);
    return result;
}

std::vector<float> FrameEnhancerBase::getInputImageData(const cv::Mat &frame) {
    std::vector<cv::Mat> bgrChannels(3);
    split(frame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels.at(c), CV_32FC1, 1 / 255.0);
    }

    const int imageArea = frame.rows * frame.cols;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize); // B
    return inputImageData;
}

cv::Mat FrameEnhancerBase::getOutputImage(const float *outputData, const cv::Size &size) {
    const long long channelStep = size.width * size.height;
    cv::Mat outputImage(size, CV_32FC3);

    // Direct memory copy and processing
    float *outputPtr = outputImage.ptr<float>();
#pragma omp parallel for
    for (long long i = 0; i < channelStep; ++i) {
        // B, G, R order for OpenCV
        outputPtr[i * 3] = std::clamp(outputData[channelStep * 2 + i] * 255.f, 0.f, 255.f); // B
        outputPtr[i * 3 + 1] = std::clamp(outputData[channelStep + i] * 255.f, 0.f, 255.f); // G
        outputPtr[i * 3 + 2] = std::clamp(outputData[i] * 255.f, 0.f, 255.f);               // R
    }

    return outputImage;
}

void FrameEnhancerBase::validateInputData(const ProcessorBase::InputData *inputData) {
    if (inputData == nullptr) {
        throw std::runtime_error(__FUNCTION__ + std::string(": inputData is null."));
    }
    if (inputData->m_targetFrame == nullptr) {
        throw std::invalid_argument(__FUNCTION__ + std::string(": inputData->m_targetFrame is nullptr"));
    }
}