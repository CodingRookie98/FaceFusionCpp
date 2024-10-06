/**
 ******************************************************************************
 * @file           : face_masker_region.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#include "face_masker_region.h"
FaceMaskerRegion::FaceMaskerRegion(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    FaceMaskerBase(env, modelPath) {
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][2];
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][3];
}

cv::Mat FaceMaskerRegion::createRegionMask(const cv::Mat &inputImage, const std::unordered_set<Region> &regions) {
    std::vector<float> inputImageData = getInputImageData(inputImage);
    std::vector<int64_t> inputImageShape{1, 3, m_inputWidth, m_inputHeight};
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
    const int outputHeight = outsShape[2];
    const int outputWidth = outsShape[3];
    const int outputArea = outputHeight * outputWidth;

    std::vector<cv::Mat> masks;
    if (regions.contains(Region::All)) {
        for (const auto &region : m_allRegions) {
            int regionInt = static_cast<int>(region);
            cv::Mat regionMask(outputHeight, outputWidth, CV_32FC1, pdata + regionInt * outputArea);
            masks.emplace_back(std::move(regionMask));
        }
    } else {
        for (const auto &region : regions) {
            int regionInt = static_cast<int>(region);
            cv::Mat regionMask(outputHeight, outputWidth, CV_32FC1, pdata + regionInt * outputArea);
            regionMask.setTo(0, regionMask < 0);
            regionMask.setTo(1, regionMask > 1);
            masks.emplace_back(std::move(regionMask));
        }
    }

    cv::Mat resultMask;
    cv::max(masks[0], masks[1], resultMask);
    for (size_t i = 2; i < masks.size(); ++i) {
        cv::max(resultMask, masks[i], resultMask);
    }

    cv::resize(resultMask, resultMask, inputImage.size());

    cv::GaussianBlur(resultMask, resultMask, cv::Size(0, 0), 5);
    resultMask.setTo(0.5, resultMask < 0.5);
    resultMask.setTo(1, resultMask > 1);

    resultMask = (resultMask - 0.5) * 2;
    return resultMask;
}

std::vector<float> FaceMaskerRegion::getInputImageData(const cv::Mat &image) {
    cv::Mat inputImage = image.clone();
    cv::resize(image, inputImage, cv::Size(m_inputHeight, m_inputWidth));
    cv::flip(inputImage, inputImage, 1);
    std::vector<cv::Mat> bgrChannels(3);
    cv::split(inputImage, bgrChannels);
    for (int i = 0; i < 3; i++) {
        bgrChannels[i].convertTo(bgrChannels[i], CV_32FC1, 1.0 / 127.5, -1.0);
    }
    const int imageArea = inputImage.cols * inputImage.rows;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize); /// rgb顺序
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize);

    return inputImageData;
}
