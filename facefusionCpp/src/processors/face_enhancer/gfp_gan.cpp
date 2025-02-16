/**
 ******************************************************************************
 * @file           : face_enhancer_gfpgan.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_enhancer;
import :gfp_gan;

namespace ffc::faceEnhancer {

GFP_GAN::GFP_GAN(const std::shared_ptr<Ort::Env>& env) :
    InferenceSession(env) {
}

std::string GFP_GAN::getProcessorName() const {
    return "FaceEnhancer.GFP_GAN";
}
void GFP_GAN::loadModel(const std::string& modelPath, const Options& options) {
    InferenceSession::loadModel(modelPath, options);
    m_inputHeight = static_cast<int>(m_inputNodeDims[0][2]);
    m_inputWidth = static_cast<int>(m_inputNodeDims[0][3]);
    m_size = cv::Size(m_inputWidth, m_inputHeight);
}

cv::Mat GFP_GAN::enhanceFace(const GFP_GAN_Input& input) const {
    if (input.targetFrame == nullptr || input.targetFaces == nullptr) {
        throw std::runtime_error("args.targetFrame or args.targetFaces is nullptr");
    }
    if (input.targetFrame->empty() || input.targetFaces->empty()) {
        throw std::runtime_error("args.targetFrame or args.targetFaces is empty");
    }
    if (input.faceBlend > 100) {
        throw std::runtime_error("args.faceBlend is greater than 100");
    }
    if (!isModelLoaded()) {
        throw std::runtime_error("model is not loaded");
    }
    if (!hasFaceMaskerHub()) {
        throw std::runtime_error("faceMaskers is nullptr");
    }

    std::vector<cv::Mat> croppedTargetFrames;
    std::vector<cv::Mat> affineMatrices;
    std::vector<cv::Mat> croppedResultFrames;
    std::vector<cv::Mat> bestMasks;
    for (const auto& targetFace : *input.targetFaces) {
        cv::Mat croppedTargetFrame, affineMatrix;
        std::tie(croppedTargetFrame, affineMatrix) = FaceHelper::warpFaceByFaceLandmarks5(*input.targetFrame, targetFace.m_landMark5By68, FaceHelper::getWarpTemplate(m_warpTemplateType), m_size);
        croppedTargetFrames.emplace_back(croppedTargetFrame);
        affineMatrices.emplace_back(affineMatrix);
    }
    for (const auto& croppedTargetFrame : croppedTargetFrames) {
        croppedResultFrames.emplace_back(applyEnhance(croppedTargetFrame));
    }
    for (auto& croppedTargetFrame : croppedTargetFrames) {
        FaceMaskerHub::Args4GetBestMask args4_get_best_mask = input.args4_get_best_mask;
        if (args4_get_best_mask.faceMaskersTypes.contains(FaceMaskerHub::Type::Region)) {
            args4_get_best_mask.faceMaskersTypes.erase(FaceMaskerHub::Type::Region);
        }
        args4_get_best_mask.boxSize = {m_size};
        args4_get_best_mask.occlusionFrame = {&croppedTargetFrame};
        bestMasks.emplace_back(m_faceMaskerHub->getBestMask(args4_get_best_mask));
    }
    if (croppedTargetFrames.size() != affineMatrices.size() || croppedTargetFrames.size() != croppedResultFrames.size() || croppedTargetFrames.size() != bestMasks.size()) {
        throw std::runtime_error("The size of croppedTargetFrames, affineMatrices, croppedResultFrames, and bestMasks must be equal.");
    }
    cv::Mat resultFrame = input.targetFrame->clone();
    for (size_t i = 0; i < bestMasks.size(); ++i) {
        resultFrame = FaceHelper::pasteBack(resultFrame, croppedResultFrames[i], bestMasks[i], affineMatrices[i]);
    }
    resultFrame = blendFrame(*input.targetFrame, resultFrame, input.faceBlend);
    return resultFrame;
}

cv::Mat GFP_GAN::applyEnhance(const cv::Mat& croppedFrame) const {
    std::vector<float> inputImageData = getInputImageData(croppedFrame);
    const std::vector<int64_t> inputDataShape{1, 3, m_inputHeight, m_inputWidth};
    std::vector<Ort::Value> inputTensors;
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo,
                                                              inputImageData.data(), inputImageData.size(),
                                                              inputDataShape.data(), inputDataShape.size()));

    std::vector<Ort::Value> outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(),
                                                             inputTensors.data(), inputTensors.size(),
                                                             m_outputNames.data(), m_outputNames.size());

    auto* pdata = outputTensor[0].GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    const int outputHeight = static_cast<int>(outsShape[2]);
    const int outputWidth = static_cast<int>(outsShape[3]);

    const long long channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Create matrices for each channel and scale/clamp values
    channelMats[2] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata);                   // R
    channelMats[1] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + channelStep);     // G
    channelMats[0] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + 2 * channelStep); // B
    for (auto& mat : channelMats) {
        mat.setTo(-1, mat < -1);
        mat.setTo(1, mat > 1);
        mat = (mat + 1) * 125.f;
        mat.setTo(0, mat < 0);
        mat.setTo(255, mat > 255);
    }
    // Merge the channels into a single matrix
    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);
    resultMat.convertTo(resultMat, CV_8UC3);

    return resultMat;
}

std::vector<float> GFP_GAN::getInputImageData(const cv::Mat& croppedImage) {
    std::vector<cv::Mat> bgrChannels(3);
    split(croppedImage, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / (255.0 * 0.5), -1.0);
    }

    const int imageArea = croppedImage.cols * croppedImage.rows;
    std::vector<float> inputImageData;
    inputImageData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float*)bgrChannels[2].data, singleChnSize); /// rgb顺序
    memcpy(inputImageData.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize);
    memcpy(inputImageData.data() + imageArea * 2, (float*)bgrChannels[0].data, singleChnSize);
    return inputImageData;
}

} // namespace ffc::faceEnhancer