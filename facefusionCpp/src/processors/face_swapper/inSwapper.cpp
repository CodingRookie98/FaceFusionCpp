/**
 ******************************************************************************
 * @file           : face_swapper_inswaper_128.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <fstream>
#include <onnx/onnx_pb.h>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_swapper;
import :inSwapper;
import face_helper;

namespace ffc::faceSwapper {

InSwapper::InSwapper(const std::shared_ptr<Ort::Env>& env) :
    FaceSwapperBase(), InferenceSession(env) {
}

void InSwapper::loadModel(const std::string& modelPath, const Options& options) {
    InferenceSession::loadModel(modelPath, options);
    m_initializerArray.clear();
    init();
}

std::string InSwapper::getProcessorName() const {
    return "FaceSwapper.InSwapper";
}

void InSwapper::init() {
    m_inputWidth = m_inputNodeDims[0][2];
    m_inputHeight = m_inputNodeDims[0][3];
    m_size = cv::Size(m_inputWidth, m_inputHeight);

    // Load ONNX model as a protobuf message
    onnx::ModelProto modelProto;
    std::ifstream input(getModelPath(), std::ios::binary);
    if (!modelProto.ParseFromIstream(&input)) {
        throw std::runtime_error("Failed to load model.");
    }
    // Access the initializer
    const onnx::TensorProto& initializer = modelProto.graph().initializer(modelProto.graph().initializer_size() - 1);
    bool isFp16 = false;

    for (auto itr = modelProto.graph().initializer().cbegin(); itr != modelProto.graph().initializer().cend();) {
        if (itr->data_type() == onnx::TensorProto_DataType::TensorProto_DataType_FLOAT16) {
            isFp16 = true;
            break;
        }
        ++itr;
    }

    // Convert initializer to an array
    if (!isFp16) {
        m_initializerArray.assign(initializer.float_data().begin(), initializer.float_data().end());
    }
    if (isFp16) {
        std::string rawData = initializer.raw_data();
        auto data = reinterpret_cast<const float*>(rawData.data());
        m_initializerArray.assign(data, data + rawData.size() / sizeof(float));
    }
    input.close();
}

cv::Mat InSwapper::swapFace(const InSwapperInput& input) {
    if (input.sourceFace == nullptr || input.targetFaces == nullptr || input.targetFrame == nullptr) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: Invalid input data.(some object is nullptr)", __FILE__, __LINE__));
    }

    if (isModelLoaded() == false) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: Model is not loaded!", __FILE__, __LINE__));
    }
    if (isModelLoaded() == true && m_initializerArray.empty()) {
        init();
    }

    if (!hasFaceMaskerHub()) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: faceMaskers is nullptr!", __FILE__, __LINE__));
    }

    const auto& sourceFace = *input.sourceFace;
    const auto& targetFaces = *input.targetFaces;
    const auto& targetFrame = *input.targetFrame;
    if (sourceFace.isEmpty() || targetFaces.empty() || targetFrame.empty()) {
        return {};
    }
    std::vector<cv::Mat> croppedTargetFrames;
    std::vector<cv::Mat> affineMatrices;
    std::vector<cv::Mat> croppedResultFrames;
    std::vector<cv::Mat> bestMasks;
    for (const auto& targetFace : targetFaces) {
        cv::Mat croppedTargetFrame, affineMat;
        std::tie(croppedTargetFrame, affineMat) = FaceHelper::warpFaceByFaceLandmarks5(targetFrame, targetFace.m_landMark5By68,
                                                                                       FaceHelper::getWarpTemplate(m_warpTemplateType),
                                                                                       m_size);
        croppedTargetFrames.emplace_back(croppedTargetFrame);
        affineMatrices.emplace_back(affineMat);
    }
    for (const auto& croppedTargetFrame : croppedTargetFrames) {
        croppedResultFrames.emplace_back(applySwap(sourceFace.m_embedding, croppedTargetFrame));
    }
    for (size_t i = 0; i < croppedTargetFrames.size(); ++i) {
        FaceMaskerHub::Args4GetBestMask args_4_get_best_mask = input.args_4_get_best_mask;
        args_4_get_best_mask.boxSize = {m_size};
        args_4_get_best_mask.occlusionFrame = {&croppedTargetFrames[i]};
        args_4_get_best_mask.regionFrame = {&croppedResultFrames[i]};
        bestMasks.emplace_back(m_faceMaskerHub->getBestMask(args_4_get_best_mask));
    }

    if (croppedTargetFrames.size() != affineMatrices.size() || croppedTargetFrames.size() != croppedResultFrames.size() || croppedTargetFrames.size() != bestMasks.size()) {
        throw std::runtime_error("The size of croppedTargetFrames, affineMatrices, croppedResultFrames, and bestMasks must be equal.");
    }

    cv::Mat resultFrame = targetFrame.clone();
    for (size_t i = 0; i < bestMasks.size(); ++i) {
        resultFrame = FaceHelper::pasteBack(resultFrame, croppedResultFrames[i], bestMasks[i], affineMatrices[i]);
    }
    return resultFrame;
}

cv::Mat InSwapper::applySwap(const Face::Embedding& sourceEmbedding, const cv::Mat& croppedTargetFrame) const {
    std::vector<Ort::Value> inputTensors;
    std::vector<float> inputImageData, inputEmbeddingData;
    for (const auto& inputName : m_inputNames) {
        if (std::string(inputName) == "source") {
            inputEmbeddingData = prepareSourceEmbedding(sourceEmbedding);
            std::vector<int64_t> inputEmbeddingShape{1, static_cast<int64_t>(inputEmbeddingData.size())};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputEmbeddingData.data(), inputEmbeddingData.size(), inputEmbeddingShape.data(), inputEmbeddingShape.size()));
        } else if (std::string(inputName) == "target") {
            inputImageData = getInputImageData(croppedTargetFrame);
            std::vector<int64_t> inputImageShape = {1, 3, m_inputHeight, m_inputWidth};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputImageData.data(), inputImageData.size(), inputImageShape.data(), inputImageShape.size()));
        }
    }

    auto outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_outputNames.data(), m_outputNames.size());

    inputImageData.clear();
    inputEmbeddingData.clear();

    float* pdata = outputTensor[0].GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    const long long outputHeight = outsShape[2];
    const long long outputWidth = outsShape[3];

    const long long channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Create matrices for each channel and scale/clamp values
    channelMats[2] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata);                   // R
    channelMats[1] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + channelStep);     // G
    channelMats[0] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + 2 * channelStep); // B
    for (auto& mat : channelMats) {
        mat *= 255.f;
        mat.setTo(0, mat < 0);
        mat.setTo(255, mat > 255);
    }
    // Merge the channels into a single matrix
    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);
    return resultMat;
}

std::vector<float> InSwapper::prepareSourceEmbedding(const Face::Embedding& sourceEmbedding) const {
    std::vector<float> result;
    const double norm = cv::norm(sourceEmbedding, cv::NORM_L2);
    const size_t lenFeature = sourceEmbedding.size();
    result.resize(lenFeature);
    for (int i = 0; i < lenFeature; ++i) {
        double sum = 0.0f;
        for (int j = 0; j < lenFeature; ++j) {
            sum += sourceEmbedding.at(j) * m_initializerArray.at(j * lenFeature + i);
        }
        result.at(i) = static_cast<float>(sum / norm);
    }
    return result;
}

std::vector<float> InSwapper::getInputImageData(const cv::Mat& croppedTargetFrame) const {
    std::vector<cv::Mat> bgrChannels(3);
    split(croppedTargetFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels.at(c), CV_32FC1, 1 / (255.0 * m_standardDeviation.at(c)),
                                 -m_mean.at(c) / static_cast<float>(m_standardDeviation.at(c)));
    }

    const int imageArea = croppedTargetFrame.rows * croppedTargetFrame.cols;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float*)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float*)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float*)bgrChannels[0].data, singleChnSize); // B
    return inputImageData;
}
}; // namespace ffc::faceSwapper