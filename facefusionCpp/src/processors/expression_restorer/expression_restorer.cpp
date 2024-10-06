/**
 ******************************************************************************
 * @file           : expression_restorer.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

#include "expression_restorer.h"
#include <future>
#include <live_portrait_helper.h>

ExpressionRestorer::ExpressionRestorer(const std::shared_ptr<Ort::Env> &env,
                                       const std::shared_ptr<FaceMaskers> &faceMaskers,
                                       const std::string &featureExtractorPath,
                                       const std::string &motionExtractorPath,
                                       const std::string &generatorPath) :
    ExpressionRestorerBase(env, faceMaskers, featureExtractorPath, motionExtractorPath, generatorPath) {
}

std::unordered_set<ProcessorBase::InputDataType> ExpressionRestorer::getInputDataTypes() {
    return {ProcessorBase::InputDataType::TargetFrame,
            ProcessorBase::InputDataType::TargetFaces,
            ProcessorBase::InputDataType::OriginalTargetFrame};
}

cv::Mat ExpressionRestorer::processFrame(const ProcessorBase::InputData *inputData) {
    ExpressionRestorerBase::validateInputData(inputData);
    
    // resize originalTargetFrame to targetFrame
    cv::Mat originalTargetFrame, resultFrame = inputData->m_targetFrame->clone();
    cv::resize(*inputData->m_originalTargetFrame, originalTargetFrame, cv::Size(inputData->m_targetFrame->cols, inputData->m_targetFrame->rows));

    for (const auto &face : *inputData->m_targetFaces) {
        if (face.isEmpty()) {
            continue;
        }
        resultFrame = restoreExpression(originalTargetFrame, resultFrame, face);
    }

    return resultFrame;
}

cv::Mat ExpressionRestorer::restoreExpression(const cv::Mat &sourceFrame, const cv::Mat &targetFrame, const Face &targetFace) {
    cv::Mat sourceCropFrame, targetCropFrame, affineMat;
    std::tie(sourceCropFrame, std::ignore) = FaceHelper::warpFaceByFaceLandmarks5(sourceFrame, targetFace.m_landMark5By68, m_warpTemplateType, m_size);
    std::tie(targetCropFrame, affineMat) = FaceHelper::warpFaceByFaceLandmarks5(targetFrame, targetFace.m_landMark5By68, m_warpTemplateType, m_size);

    std::vector<std::future<cv::Mat>> futures;
    static cv::Mat (FaceMaskers:: *funcPtrBox)(const cv::Size &) = &FaceMaskers::createStaticBoxMask;
    futures.emplace_back(std::async(std::launch::async, funcPtrBox, m_faceMaskers, targetCropFrame.size()));
    if (m_maskerTypes.contains(FaceMaskers::Occlusion)) {
        futures.emplace_back(std::async(std::launch::async, &FaceMaskers::createOcclusionMask, m_faceMaskers, targetCropFrame));
    }

    std::future<std::vector<float>> featureVolumeFu = std::async(std::launch::async, &ExpressionRestorer::forwardExtractFeature, this, targetCropFrame);
    std::future<std::vector<std::vector<float>>> sourceMotionFu = std::async(std::launch::async, &ExpressionRestorer::forwardExtractMotion, this, sourceCropFrame);

    std::vector<std::vector<float>> targetMotion = forwardExtractMotion(targetCropFrame);

    std::vector<float> featureVolume = featureVolumeFu.get();
    std::vector<std::vector<float>> sourceMotion = sourceMotionFu.get();

    cv::Mat rotationMat = LivePortraitHelper::createRotationMat(targetMotion[0][0], targetMotion[1][0], targetMotion[2][0]);
    std::vector<float> sourceExpression = std::move(sourceMotion[5]), targetExpression = std::move(targetMotion[5]);
    for (auto &index : std::array<int, 5>{0, 4, 5, 8, 9}) {
        sourceExpression[index] = targetExpression[index];
    }

    cv::Mat sourceExpressionMat(21, 3, CV_32FC1, sourceExpression.data());
    cv::Mat targetExpressionMat(21, 3, CV_32FC1, targetExpression.data());

    sourceExpressionMat = sourceExpressionMat * m_restoreFactor + targetExpressionMat * (1 - m_restoreFactor);
    sourceExpressionMat = LivePortraitHelper::limitExpression(sourceExpressionMat);

    cv::Mat targetTranslationMat(21, 3, CV_32FC1);
    for (int i = 0; i < 21; ++i) {
        for (int j = 0; j < 3; ++j) {
            targetTranslationMat.at<float>(i, j) = targetMotion[4][j];
        }
    }
    float targetScale = targetMotion[3][0];

    cv::Mat targetMotionPoints(21, 3, CV_32FC1, targetMotion[6].data());
    cv::Mat sourceMotionPoints = targetScale * (targetMotionPoints * rotationMat.t() + sourceExpressionMat) + targetTranslationMat;
    targetMotionPoints = targetScale * (targetMotionPoints * rotationMat.t() + targetExpressionMat) + targetTranslationMat;

    std::vector<float> sourceMotionPointsData, targetMotionPointsData;
    sourceMotionPointsData.assign(sourceMotionPoints.begin<float>(), sourceMotionPoints.end<float>());
    targetMotionPointsData.assign(targetMotionPoints.begin<float>(), targetMotionPoints.end<float>());

    cv::Mat result = forwardGenerateFrame(featureVolume, sourceMotionPointsData, targetMotionPointsData);

    std::vector<cv::Mat> masks;
    for (auto &future : futures) {
        masks.emplace_back(future.get());
    }
    cv::Mat bestMask = FaceMaskers::getBestMask(masks);
    result = FaceHelper::pasteBack(targetFrame, result, bestMask, affineMat);
    return result;
}

std::vector<float> ExpressionRestorer::getInputImageData(const cv::Mat &image) {
    cv::Mat inputImage;
    cv::resize(image, inputImage, m_size / 2, cv::InterpolationFlags::INTER_AREA);
    std::vector<cv::Mat> bgrChannels(3);
    cv::split(inputImage, bgrChannels);
    for (int i = 0; i < 3; ++i) {
        bgrChannels[i].convertTo(bgrChannels[i], CV_32FC1, 1.0 / 255.0);
    }

    const int imageArea = inputImage.cols * inputImage.rows;
    std::vector<float> inputImageData(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize); // B
    return inputImageData;
}

std::vector<float> ExpressionRestorer::forwardExtractFeature(const cv::Mat &image) {
    std::vector<float> inputImageData = getInputImageData(image);
    std::vector<Ort::Value> inputTensors;
    std::vector<int64_t> inputShape = {1, 3, m_size.height / 2, m_size.width / 2};
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_featureSession.m_memoryInfo, inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

    auto outputTensor = m_featureSession.m_ortSession->Run(m_featureSession.m_runOptions, m_featureSession.m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_featureSession.m_outputNames.data(), m_featureSession.m_outputNames.size());
    inputImageData.clear();

    auto *outputData = outputTensor.front().GetTensorMutableData<float>();
    size_t outputSize = 1 * 32 * 16 * 64 * 64;
    std::vector<float> outputDataVec(outputData, outputData + outputSize);

    return outputDataVec;
}

std::vector<std::vector<float>> ExpressionRestorer::forwardExtractMotion(const cv::Mat &image) {
    std::vector<float> inputImageData = getInputImageData(image);
    std::vector<Ort::Value> inputTensors;
    std::vector<int64_t> inputShape = {1, 3, m_size.height / 2, m_size.width / 2};
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_motionSession.m_memoryInfo, inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

    auto outputTensor = m_motionSession.m_ortSession->Run(m_motionSession.m_runOptions, m_motionSession.m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_motionSession.m_outputNames.data(), m_motionSession.m_outputNames.size());
    inputImageData.clear();

    std::vector<std::vector<float>> outputDataVec;
    for (size_t i = 0; i < m_motionSession.m_outputNames.size(); ++i) {
        auto *outputData = outputTensor[i].GetTensorMutableData<float>();
        if (i == 0 || i == 1 || i == 2 || i == 3) {
            outputDataVec.emplace_back(outputData, outputData + 1);
        }
        if (i == 4) {
            outputDataVec.emplace_back(outputData, outputData + (1 * 3));
        }
        if (i == 5 || i == 6) {
            outputDataVec.emplace_back(outputData, outputData + (1 * 21 * 3));
        }
    }

    return outputDataVec;
}

cv::Mat ExpressionRestorer::forwardGenerateFrame(std::vector<float> &featureVolume,
                                                 std::vector<float> &sourceMotionPoints,
                                                 std::vector<float> &targetMotionPoints) {
    std::vector<Ort::Value> inputTensors;
    std::vector<int64_t> inputFeatureShape{1, 32, 16, 64, 64};
    std::vector<int64_t> inputMotionShape{1, 21, 3};
    for (const auto &name : m_generatorSession.m_inputNames) {
        std::string nameStr(name);
        if (nameStr == "feature_volume") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_generatorSession.m_memoryInfo, featureVolume.data(), featureVolume.size(), inputFeatureShape.data(), inputFeatureShape.size()));
        }
        if (nameStr == "source") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_generatorSession.m_memoryInfo, sourceMotionPoints.data(), sourceMotionPoints.size(), inputMotionShape.data(), inputMotionShape.size()));
        }
        if (nameStr == "target") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_generatorSession.m_memoryInfo, targetMotionPoints.data(), targetMotionPoints.size(), inputMotionShape.data(), inputMotionShape.size()));
        }
    }

    auto outputTensor = m_generatorSession.m_ortSession->Run(m_generatorSession.m_runOptions, m_generatorSession.m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_generatorSession.m_outputNames.data(), m_generatorSession.m_outputNames.size());

    auto *outputData = outputTensor.front().GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    long long outputHeight = outsShape[2];
    long long outputWidth = outsShape[3];

    long long channelStep = outputHeight * outputWidth;
    std::vector<cv::Mat> channelMats(3);
    // Create matrices for each channel and scale/clamp values
    channelMats[2] = cv::Mat(outputHeight, outputWidth, CV_32FC1, outputData);                   // R
    channelMats[1] = cv::Mat(outputHeight, outputWidth, CV_32FC1, outputData + channelStep);     // G
    channelMats[0] = cv::Mat(outputHeight, outputWidth, CV_32FC1, outputData + 2 * channelStep); // B
    for (auto &mat : channelMats) {
        mat *= 255.f;
        mat.setTo(0, mat < 0);
        mat.setTo(255, mat > 255);
    }
    cv::Mat resultMat;
    cv::merge(channelMats, resultMat);
    return resultMat;
}
