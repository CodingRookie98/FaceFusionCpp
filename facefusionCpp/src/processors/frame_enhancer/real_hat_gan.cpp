/**
 ******************************************************************************
 * @file           : real_hat_gan.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

#include "real_hat_gan.h"
#include "vision.h"

RealHatGan::RealHatGan(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    FrameEnhancerBase(env, modelPath) {
}

cv::Mat RealHatGan::processFrame(const ProcessorBase::InputData *inputData) {
    FrameEnhancerBase::validateInputData(inputData);

    if (inputData->m_targetFrame->empty()) {
        return *inputData->m_targetFrame;
    }

    return enhanceFrame(*inputData->m_targetFrame);
}

std::unordered_set<ProcessorBase::InputDataType> RealHatGan::getInputDataTypes() {
    return {ProcessorBase::InputDataType::TargetFrame};
}

cv::Mat RealHatGan::enhanceFrame(const cv::Mat &frame) const {
    int tempWidth = frame.cols, tempHeight = frame.rows;
    auto [tileVisionFrames, padWidth, pidHeight] = Ffc::Vision::createTileFrames(frame, m_tileSize);

    for (size_t i = 0; i < tileVisionFrames.size(); i++) {
        std::vector<float> inputImageData = getInputImageData(tileVisionFrames[i]);
        std::vector<int64_t> inputShape{1, 3, tileVisionFrames[i].cols, tileVisionFrames[i].rows};
        std::vector<Ort::Value> inputTensors;
        inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

        std::vector<Ort::Value> outputTensor = m_inferenceSession.m_ortSession->Run(m_inferenceSession.m_runOptions,
                                                                                    m_inferenceSession.m_inputNames.data(),
                                                                                    inputTensors.data(), inputTensors.size(),
                                                                                    m_inferenceSession.m_outputNames.data(),
                                                                                    m_inferenceSession.m_outputNames.size());
        float *outputData = outputTensor[0].GetTensorMutableData<float>();
        int outputWidth = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[2];
        int outputHeight = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[3];
        cv::Mat outputImage = getOutputImage(outputData, cv::Size(outputWidth, outputHeight));
        tileVisionFrames[i] = outputImage;
    }

    cv::Mat outputImage = Ffc::Vision::mergeTileFrames(tileVisionFrames, tempWidth * m_modelScale, tempHeight * m_modelScale,
                                                       padWidth * m_modelScale, pidHeight * m_modelScale,
                                                       {m_tileSize[0] * m_modelScale, m_tileSize[1] * m_modelScale, m_tileSize[2] * m_modelScale});

    outputImage = blendFrame(frame, outputImage);
    return outputImage;
}
