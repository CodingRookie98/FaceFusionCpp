/**
 ******************************************************************************
 * @file           : face_swapper_inswaper_128.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "face_swapper_inswaper_128.h"
#include <fstream>
#include <future>

FaceSwapperInswaper128::FaceSwapperInswaper128(const std::shared_ptr<Ort::Env> &env, const std::shared_ptr<FaceMaskers> &faceMaskers, const std::string &modelPath, const Version &version) :
    FaceSwapperBase(env, faceMaskers, modelPath) {
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][2];
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][3];

    // Load ONNX model as a protobuf message
    onnx::ModelProto modelProto;
    std::ifstream input(modelPath, std::ios::binary);
    if (!modelProto.ParseFromIstream(&input)) {
        throw std::runtime_error("Failed to load model.");
    }
    // Access the initializer
    const onnx::TensorProto &initializer = modelProto.graph().initializer(modelProto.graph().initializer_size() - 1);

    // Convert initializer to an array
    if (version == V128) {
        m_initializerArray.assign(initializer.float_data().begin(), initializer.float_data().end());
    }
    if (version == V128_fp16) {
        std::string rawData = initializer.raw_data();
        auto data = reinterpret_cast<const float *>(rawData.data());
        m_initializerArray.assign(data, data + rawData.size() / sizeof(float));
    }
    input.close();
}

std::unordered_set<ProcessorBase::InputDataType> FaceSwapperInswaper128::getInputDataTypes() {
    return {ProcessorBase::InputDataType::SourceFaces,
            ProcessorBase::InputDataType::TargetFaces,
            ProcessorBase::InputDataType::TargetFrame};
}

cv::Mat FaceSwapperInswaper128::processFrame(const ProcessorBase::InputData *inputData) {
    FaceSwapperBase::validateInputData(inputData);

    cv::Mat resultFrame = inputData->m_targetFrame->clone();
    for (const auto &targetFace : *inputData->m_targetFaces) {
        if (targetFace.isEmpty()) {
            continue;
        }
        resultFrame = swapFace(inputData->m_sourceFaces->front(), targetFace, resultFrame);
    }

    return resultFrame;
}

cv::Mat FaceSwapperInswaper128::swapFace(const Face &sourceFace, const Face &targetFace, const cv::Mat &targetFrame) {
    cv::Mat croppedTargetFrame, affineMat;
    std::tie(croppedTargetFrame, affineMat) = FaceHelper::warpFaceByFaceLandmarks5(targetFrame, targetFace.m_landMark5By68,
                                                                                   FaceHelper::getWarpTemplate(m_warpTemplateType),
                                                                                   m_size);
    std::vector<std::future<cv::Mat>> futures;
    static cv::Mat (FaceMaskers:: *funcPtrBox)(const cv::Size &) = &FaceMaskers::createStaticBoxMask;
    futures.emplace_back(std::async(std::launch::async, funcPtrBox, m_faceMaskers, croppedTargetFrame.size()));
    if (m_maskerTypes.contains(FaceMaskers::Occlusion)) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskers::createOcclusionMask, m_faceMaskers, croppedTargetFrame));
    }

    std::vector<Ort::Value> inputTensors;
    std::vector<float> inputImageData, inputEmbeddingData;
    for (const auto &inputName : m_inferenceSession.m_inputNames) {
        if (std::string(inputName) == "source") {
            inputEmbeddingData = prepareSourceEmbedding(sourceFace);
            std::vector<int64_t> inputEmbeddingShape = {1, (int64_t)inputEmbeddingData.size()};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputEmbeddingData.data(), inputEmbeddingData.size(), inputEmbeddingShape.data(), inputEmbeddingShape.size()));
        } else if (std::string(inputName) == "target") {
            inputImageData = getInputImageData(croppedTargetFrame);
            std::vector<int64_t> inputImageShape = {1, 3, m_inputHeight, m_inputWidth};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputImageData.data(), inputImageData.size(), inputImageShape.data(), inputImageShape.size()));
        }
    }

    auto outputTensor = m_inferenceSession.m_ortSession->Run(m_inferenceSession.m_runOptions, m_inferenceSession.m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_inferenceSession.m_outputNames.data(), m_inferenceSession.m_outputNames.size());

    inputImageData.clear();
    inputEmbeddingData.clear();

    float *pdata = outputTensor[0].GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    long long outputHeight = outsShape[2];
    long long outputWidth = outsShape[3];

    long long channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Create matrices for each channel and scale/clamp values
    channelMats[2] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata);                   // R
    channelMats[1] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + channelStep);     // G
    channelMats[0] = cv::Mat(outputHeight, outputWidth, CV_32FC1, pdata + 2 * channelStep); // B
    for (auto &mat : channelMats) {
        mat *= 255.f;
        mat.setTo(0, mat < 0);
        mat.setTo(255, mat > 255);
    }
    // Merge the channels into a single matrix
    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);

    if (m_maskerTypes.contains(FaceMaskers::Type::Region)) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskers::createOcclusionMask, m_faceMaskers, resultMat));
    }
    std::vector<cv::Mat> masks;
    for (auto &future : futures) {
        masks.emplace_back(future.get());
    }
    for (auto &cropMask : masks) {
        cropMask.setTo(0, cropMask < 0);
        cropMask.setTo(1, cropMask > 1);
    }

    auto bestCropMask = m_faceMaskers->getBestMask(masks);

    resultMat = FaceHelper::pasteBack(targetFrame, resultMat, bestCropMask, affineMat);

    return resultMat;
}

std::vector<float> FaceSwapperInswaper128::prepareSourceEmbedding(const Face &sourceFace) const {
    std::vector<float> result;
    double norm = cv::norm(sourceFace.m_embedding, cv::NORM_L2);
    size_t lenFeature = sourceFace.m_embedding.size();
    result.resize(lenFeature);
    for (int i = 0; i < lenFeature; ++i) {
        double sum = 0.0f;
        for (int j = 0; j < lenFeature; ++j) {
            sum += sourceFace.m_embedding.at(j)
                   * m_initializerArray.at(j * lenFeature + i);
        }
        result.at(i) = (float)(sum / norm);
    }
    return result;
}

std::vector<float> FaceSwapperInswaper128::getInputImageData(const cv::Mat &cropFrame) const {
    std::vector<cv::Mat> bgrChannels(3);
    split(cropFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels.at(c), CV_32FC1, 1 / (255.0 * m_standardDeviation.at(c)),
                                 -m_mean.at(c) / (float)m_standardDeviation.at(c));
    }

    const int imageArea = cropFrame.rows * cropFrame.cols;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize); // B
    return inputImageData;
}
