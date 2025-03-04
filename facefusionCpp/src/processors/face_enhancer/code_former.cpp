/**
 ******************************************************************************
 * @file           : code_former.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-22
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_enhancer;
import :code_former;

namespace ffc::faceEnhancer {
CodeFormer::CodeFormer(const std::shared_ptr<Ort::Env>& env) :
    InferenceSession(env) {
}

std::string CodeFormer::getProcessorName() const {
    return "FaceEnhancer.CodeFormer";
}

void CodeFormer::LoadModel(const std::string& modelPath, const Options& options) {
    InferenceSession::LoadModel(modelPath, options);
    m_inputHeight = input_node_dims_[0][2];
    m_inputWidth = input_node_dims_[0][3];
    m_size = cv::Size(m_inputWidth, m_inputHeight);
}

cv::Mat CodeFormer::enhanceFace(const CodeFormerInput& input) const {
    if (input.target_frame == nullptr) {
        return {};
    }
    if (input.target_frame->empty()) {
        return {};
    }
    if (input.target_faces_5_landmarks.empty()) {
        return input.target_frame->clone();
    }

    if (!IsModelLoaded()) {
        throw std::runtime_error("model is not loaded");
    }
    if (!hasFaceMaskerHub()) {
        throw std::runtime_error("faceMaskers is nullptr");
    }

    std::vector<cv::Mat> croppedTargetFrames;
    std::vector<cv::Mat> affineMatrices;
    std::vector<cv::Mat> croppedResultFrames;
    std::vector<cv::Mat> bestMasks;
    for (const auto& target_faces_5_landmark : input.target_faces_5_landmarks) {
        cv::Mat croppedTargetFrame, affineMatrix;
        std::tie(croppedTargetFrame, affineMatrix) = face_helper::warpFaceByFaceLandmarks5(*input.target_frame, target_faces_5_landmark, face_helper::getWarpTemplate(m_warpTemplateType), m_size);
        croppedTargetFrames.emplace_back(croppedTargetFrame);
        affineMatrices.emplace_back(affineMatrix);
    }
    for (const auto& croppedTargetFrame : croppedTargetFrames) {
        croppedResultFrames.emplace_back(applyEnhance(croppedTargetFrame));
    }
    for (auto& croppedTargetFrame : croppedTargetFrames) {
        FaceMaskerHub::ArgsForGetBestMask args4_get_best_mask = input.args_for_get_best_mask;
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
    cv::Mat resultFrame = input.target_frame->clone();
    for (size_t i = 0; i < bestMasks.size(); ++i) {
        resultFrame = face_helper::pasteBack(resultFrame, croppedResultFrames[i], bestMasks[i], affineMatrices[i]);
    }
    if (input.faceBlend > 100) {
        resultFrame = blendFrame(*input.target_frame, resultFrame, 100);
    } else {
        resultFrame = blendFrame(*input.target_frame, resultFrame, input.faceBlend);
    }
    return resultFrame;
}

cv::Mat CodeFormer::applyEnhance(const cv::Mat& croppedFrame) const {
    std::vector<float> inputImageData = getInputImageData(croppedFrame);
    const std::vector<int64_t> inputImageDataShape{1, 3, m_inputHeight, m_inputWidth};
    std::vector<double> inputWeightData{1.0};
    const std::vector<int64_t> inputWeightDataShape{1, 1};
    std::vector<Ort::Value> inputTensors;
    for (const auto& name : input_names_) {
        if (std::string(name) == "input") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info_->GetConst(),
                                                                      inputImageData.data(), inputImageData.size(),
                                                                      inputImageDataShape.data(), inputImageDataShape.size()));
        } else if (std::string(name) == "weight") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<double>(memory_info_->GetConst(),
                                                                       inputWeightData.data(), inputWeightData.size(),
                                                                       inputWeightDataShape.data(), inputWeightDataShape.size()));
        }
    }

    std::vector<Ort::Value> outputTensor = ort_session_->Run(run_options_,
                                                             input_names_.data(),
                                                             inputTensors.data(), inputTensors.size(),
                                                             output_names_.data(),
                                                             output_names_.size());

    auto* pdata = outputTensor[0].GetTensorMutableData<float>();
    const std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    const long long outputHeight = outsShape[2];
    const long long outputWidth = outsShape[3];

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

std::vector<float> CodeFormer::getInputImageData(const cv::Mat& croppedImage) {
    std::vector<cv::Mat> bgrChannels(3);
    split(croppedImage, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / (255.0 * 0.5), -1.0);
    }

    const int imageArea = croppedImage.cols * croppedImage.rows;
    std::vector<float> inputImageData;
    inputImageData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), bgrChannels[2].data, singleChnSize); /// rgb顺序
    memcpy(inputImageData.data() + imageArea, bgrChannels[1].data, singleChnSize);
    memcpy(inputImageData.data() + imageArea * 2, bgrChannels[0].data, singleChnSize);
    return inputImageData;
}
} // namespace ffc::faceEnhancer