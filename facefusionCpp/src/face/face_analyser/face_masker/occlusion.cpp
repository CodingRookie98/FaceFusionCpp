/**
 ******************************************************************************
 * @file           : face_masker_occlusion.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_masker_hub;
import :occlusion;

namespace ffc::faceMasker {
Occlusion::Occlusion(const std::shared_ptr<Ort::Env> &env) :
    FaceMaskerBase(env) {
}

void Occlusion::LoadModel(const std::string &modelPath, const Options &options) {
    FaceMaskerBase::LoadModel(modelPath, options);
    m_inputHeight = static_cast<int>(input_node_dims_[0][1]);
    m_inputWidth = static_cast<int>(input_node_dims_[0][2]);
}

cv::Mat Occlusion::createOcclusionMask(const cv::Mat &cropVisionFrame) const {
    std::vector<float> inputImageData = getInputImageData(cropVisionFrame);

    std::vector<int64_t> inputImageShape{1, m_inputHeight, m_inputWidth, 3};
    std::vector<Ort::Value> inputTensors;
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info_->GetConst(), inputImageData.data(),
                                                              inputImageData.size(),
                                                              inputImageShape.data(),
                                                              inputImageShape.size()));

    std::vector<Ort::Value> outputTensors = ort_session_->Run(run_options_, input_names_.data(),
                                                              inputTensors.data(), inputTensors.size(),
                                                              output_names_.data(), output_names_.size());

    auto *pdata = outputTensors[0].GetTensorMutableData<float>();
    const std::vector<int64_t> outsShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int outputHeight = static_cast<int>(outsShape[1]);
    const int outputWidth = static_cast<int>(outsShape[2]);

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

std::vector<float> Occlusion::getInputImageData(const cv::Mat &cropVisionFrame) const {
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
}