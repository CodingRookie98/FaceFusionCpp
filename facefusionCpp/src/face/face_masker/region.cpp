/**
 ******************************************************************************
 * @file           : face_masker_region.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_masker_hub;
import :region;

namespace ffc::face_masker {

FaceMaskerRegion::FaceMaskerRegion(const std::shared_ptr<Ort::Env>& env) :
    FaceMaskerBase(env) {
}

void FaceMaskerRegion::load_model(const std::string& modelPath, const Options& options) {
    FaceMaskerBase::load_model(modelPath, options);
    m_inputHeight = m_input_node_dims[0][2];
    m_inputWidth = m_input_node_dims[0][3];
}

cv::Mat FaceMaskerRegion::createRegionMask(const cv::Mat& inputImage, const std::unordered_set<Region>& regions) const {
    std::vector<float> inputImageData = getInputImageData(inputImage);
    std::vector<int64_t> inputImageShape{1, 3, m_inputWidth, m_inputHeight};
    std::vector<Ort::Value> inputTensors;
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memory_info->GetConst(), inputImageData.data(),
                                                              inputImageData.size(),
                                                              inputImageShape.data(),
                                                              inputImageShape.size()));

    std::vector<Ort::Value> outputTensors = m_ort_session->Run(m_run_options, m_input_names.data(),
                                                               inputTensors.data(), inputTensors.size(),
                                                               m_output_names.data(), m_output_names.size());

    auto* pdata = outputTensors[0].GetTensorMutableData<float>();
    const std::vector<int64_t> outsShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int outputHeight = outsShape[2];
    const int outputWidth = outsShape[3];
    const int outputArea = outputHeight * outputWidth;

    std::vector<cv::Mat> masks;
    for (const auto& region : regions) {
        int regionInt = static_cast<int>(region);
        cv::Mat regionMask(outputHeight, outputWidth, CV_32FC1, pdata + regionInt * outputArea);
        regionMask.setTo(0, regionMask < 0);
        regionMask.setTo(1, regionMask > 1);
        masks.emplace_back(std::move(regionMask));
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

std::vector<float> FaceMaskerRegion::getInputImageData(const cv::Mat& image) const {
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
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float*)bgrChannels[2].data, singleChnSize); /// rgb顺序
    memcpy(inputImageData.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize);
    memcpy(inputImageData.data() + imageArea * 2, (float*)bgrChannels[0].data, singleChnSize);

    return inputImageData;
}
} // namespace ffc::face_masker