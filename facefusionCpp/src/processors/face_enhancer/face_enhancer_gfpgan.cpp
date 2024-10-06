/**
 ******************************************************************************
 * @file           : face_enhancer_gfpgan.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

#include "face_enhancer_gfpgan.h"
#include <future>

FaceEnhancerGfpgan::FaceEnhancerGfpgan(const std::shared_ptr<Ort::Env> &env, const std::shared_ptr<FaceMaskers> &faceMaskers, const std::string &modelPath) :
    FaceEnhancerBase(env, faceMaskers, modelPath) {
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][2];
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][3];
}

std::unordered_set<ProcessorBase::InputDataType> FaceEnhancerGfpgan::getInputDataTypes() {
    return {ProcessorBase::InputDataType::TargetFrame,
            ProcessorBase::InputDataType::TargetFaces};
}

cv::Mat FaceEnhancerGfpgan::processFrame(const ProcessorBase::InputData *inputData) {
    FaceEnhancerBase::validateInputData(inputData);

    cv::Mat resultFrame = inputData->m_targetFrame->clone();
    for (const auto &targetFace : *inputData->m_targetFaces) {
        if (targetFace.isEmpty()) {
            continue;
        }
        resultFrame = enhanceFace(resultFrame, targetFace);
    }
    return resultFrame;
}

std::vector<float> FaceEnhancerGfpgan::getInputImageData(const cv::Mat &croppedImage) const {
    std::vector<cv::Mat> bgrChannels(3);
    split(croppedImage, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / (255.0 * 0.5), -1.0);
    }

    const int imageArea = croppedImage.cols * croppedImage.rows;
    std::vector<float> inputImageData;
    inputImageData.resize(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize); /// rgb顺序
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize);
    return inputImageData;
}

cv::Mat FaceEnhancerGfpgan::enhanceFace(const cv::Mat &image, const Face &targetFace) const {
    cv::Mat cropedTargetImg, affineMat;
    std::tie(cropedTargetImg, affineMat) = FaceHelper::warpFaceByFaceLandmarks5(image, targetFace.m_landMark5By68, FaceHelper::getWarpTemplate(m_warpTemplateType), m_size);

    std::vector<std::future<cv::Mat>> futures;
    static cv::Mat (FaceMaskers:: *funcPtrBox)(const cv::Size &) = &FaceMaskers::createStaticBoxMask;
    futures.emplace_back(std::async(std::launch::async, funcPtrBox, m_faceMaskers, cropedTargetImg.size()));
    if (m_maskerTypes.contains(FaceMaskers::Occlusion)) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskers::createOcclusionMask, m_faceMaskers, cropedTargetImg));
    }

    std::vector<float> inputImageData = getInputImageData(cropedTargetImg);
    std::vector<int64_t> inputDataShape{1, 3, m_inputHeight, m_inputWidth};
    std::vector<Ort::Value> inputTensors;
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo,
                                                              inputImageData.data(), inputImageData.size(),
                                                              inputDataShape.data(), inputDataShape.size()));

    std::vector<Ort::Value> outputTensor =
        m_inferenceSession.m_ortSession->Run(m_inferenceSession.m_runOptions,
                                             m_inferenceSession.m_inputNames.data(),
                                             inputTensors.data(), inputTensors.size(),
                                             m_inferenceSession.m_outputNames.data(),
                                             m_inferenceSession.m_outputNames.size());

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

    std::vector<cv::Mat> masks;
    for (auto &future : futures) {
        masks.emplace_back(future.get());
    }
    for (auto &mask : masks) {
        mask.setTo(0, mask < 0);
        mask.setTo(1, mask > 1);
    }
    auto bestMask = FaceMaskers::getBestMask(masks);

    resultMat = FaceHelper::pasteBack(image, resultMat, bestMask, affineMat);
    resultMat = blendFrame(image, resultMat);

    return resultMat;
}
