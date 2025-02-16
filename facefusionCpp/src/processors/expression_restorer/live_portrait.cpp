/**
 ******************************************************************************
 * @file           : expression_restorer.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

module;
#include <future>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module expression_restorer;
import :live_portrait;

namespace ffc::expressionRestore {
LivePortrait::LivePortrait(const std::shared_ptr<Ort::Env> &env) :
    m_featureExtractor(env), m_motionExtractor(env), m_generator(env) {
    m_env = env;
}

std::string LivePortrait::getProcessorName() const {
    return "ExpressionRestorer.LivePortrait";
}

void LivePortrait::loadModel(const std::string &featureExtractorPath, const std::string &motionExtractorPath, const std::string &generatorPath, const ffc::InferenceSession::Options &options) {
    m_featureExtractor.loadModel(featureExtractorPath, options);
    m_motionExtractor.loadModel(motionExtractorPath, options);
    m_generator.loadModel(generatorPath, options);
    m_generatorOutputSize = m_generator.getOutputSize();
}

cv::Mat LivePortrait::restoreExpression(const LivePortraitInput &input) {
    if (input.sourceFrame == nullptr || input.targetFrame == nullptr || input.targetFaces == nullptr) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: args.sourceFrame or args.targetFrame or  args.targetFaces is nullptr!", __FILE__, __LINE__));
    }
    if (input.sourceFrame->empty() || input.targetFrame->empty() || input.targetFaces->empty()) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: args.sourceFrame or args.targetFrame or  args.targetFaces is empty!", __FILE__, __LINE__));
    }

    if (!m_featureExtractor.isModelLoaded() || !m_motionExtractor.isModelLoaded() || !m_generator.isModelLoaded()) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: models is not loaded!", __FILE__, __LINE__));
    }

    if (!hasFaceMaskers()) {
        throw std::runtime_error(std::format("File: {}, Line: {}, Error: faceMaskers is nullptr!", __FILE__, __LINE__));
    }

    std::vector<cv::Mat> croppedSourceFrames, croppedTargetFrames, affineMatrices, bestMasks, restoredFrames;
    for (const auto &face : *input.targetFaces) {
        cv::Mat sourceCroppedFrame, targetCropFrame, affineMat;
        std::tie(sourceCroppedFrame, std::ignore) = FaceHelper::warpFaceByFaceLandmarks5(*input.sourceFrame, face.m_landMark5By68, m_warpTemplateType, m_generatorOutputSize);
        std::tie(targetCropFrame, affineMat) = FaceHelper::warpFaceByFaceLandmarks5(*input.targetFrame, face.m_landMark5By68, m_warpTemplateType, m_generatorOutputSize);
        croppedSourceFrames.emplace_back(sourceCroppedFrame);
        croppedTargetFrames.emplace_back(targetCropFrame);
        affineMatrices.emplace_back(affineMat);
    }
    for (auto & croppedTargetFrame : croppedTargetFrames) {
        FaceMaskerHub::Args4GetBestMask func_gbm_args;
        func_gbm_args.faceMaskersTypes = input.faceMaskersTypes;
        func_gbm_args.boxMaskBlur = input.boxMaskBlur;
        func_gbm_args.boxMaskPadding = input.boxMaskPadding;
        func_gbm_args.boxSize = m_generatorOutputSize;
        func_gbm_args.occlusionFrame = &croppedTargetFrame;
        bestMasks.emplace_back(m_faceMaskerHub->getBestMask(func_gbm_args));
    }

    if (croppedSourceFrames.size() != croppedTargetFrames.size() || croppedSourceFrames.size() != bestMasks.size() || croppedSourceFrames.size() != affineMatrices.size()) {
        throw std::runtime_error("croppedSourceFrames.size() != croppedTargetFrames.size() || croppedSourceFrames.size() != bestMasks.size() || croppedSourceFrames.size() != affineMatrices.size()");
    }

    for (size_t i = 0; i < croppedSourceFrames.size(); ++i) {
        restoredFrames.emplace_back(applyRestore(croppedSourceFrames[i], croppedTargetFrames[i]));
    }

    cv::Mat resultFrame = input.targetFrame->clone();
    for (size_t i = 0; i < croppedTargetFrames.size(); ++i) {
        resultFrame = FaceHelper::pasteBack(resultFrame, restoredFrames[i], bestMasks[i], affineMatrices[i]);
    }
    return resultFrame;
}

cv::Mat LivePortrait::applyRestore(const cv::Mat &croppedSourceFrame, const cv::Mat &croppedTargetFrame) const {
    std::future<std::vector<float>> featureVolumeFu = std::async(std::launch::async, &FeatureExtractor::extractFeature, &m_featureExtractor, croppedTargetFrame);
    std::future<std::vector<std::vector<float>>> sourceMotionFu = std::async(std::launch::async, &MotionExtractor::extractMotion, &m_motionExtractor, croppedSourceFrame);

    std::vector<std::vector<float>> targetMotion = m_motionExtractor.extractMotion(croppedTargetFrame);

    std::vector<float> featureVolume = featureVolumeFu.get();
    std::vector<std::vector<float>> sourceMotion = sourceMotionFu.get();

    cv::Mat rotationMat = createRotationMat(targetMotion[0][0], targetMotion[1][0], targetMotion[2][0]);
    std::vector<float> sourceExpression = std::move(sourceMotion[5]), targetExpression = std::move(targetMotion[5]);
    for (auto &index : std::array{0, 4, 5, 8, 9}) {
        sourceExpression[index] = targetExpression[index];
    }

    cv::Mat sourceExpressionMat(21, 3, CV_32FC1, sourceExpression.data());
    cv::Mat targetExpressionMat(21, 3, CV_32FC1, targetExpression.data());

    sourceExpressionMat = sourceExpressionMat * m_restoreFactor + targetExpressionMat * (1 - m_restoreFactor);
    sourceExpressionMat = limitExpression(sourceExpressionMat);

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

    cv::Mat result = m_generator.generateFrame(featureVolume, sourceMotionPointsData, targetMotionPointsData);

    return result;
}

LivePortrait::FeatureExtractor::FeatureExtractor(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}

std::vector<float> LivePortrait::FeatureExtractor::extractFeature(const cv::Mat &frame) const {
    if (!isModelLoaded()) {
        throw std::runtime_error("Model is not loaded");
    }
    const int &width = m_inputNodeDims[0][2], &height = m_inputNodeDims[0][3];
    std::vector<float> inputImageData = getInputImageData(frame, cv::Size(width, height));
    std::vector<Ort::Value> inputTensors;
    const std::vector<int64_t> inputShape = {1, 3, width, height};
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

    auto outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_outputNames.data(), m_outputNames.size());
    inputImageData.clear();

    auto *outputData = outputTensor.front().GetTensorMutableData<float>();
    constexpr size_t outputSize = 1 * 32 * 16 * 64 * 64;
    std::vector outputDataVec(outputData, outputData + outputSize);

    return outputDataVec;
}

LivePortrait::MotionExtractor::MotionExtractor(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}

std::vector<std::vector<float>> LivePortrait::MotionExtractor::extractMotion(const cv::Mat &frame) const {
    const int &width = m_inputNodeDims[0][2], &height = m_inputNodeDims[0][3];
    std::vector<float> inputImageData = getInputImageData(frame, cv::Size(width, height));
    std::vector<Ort::Value> inputTensors;
    const std::vector<int64_t> inputShape = {1, 3, width, height};
    inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

    auto outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_outputNames.data(), m_outputNames.size());
    inputImageData.clear();

    std::vector<std::vector<float>> outputDataVec;
    for (size_t i = 0; i < m_outputNames.size(); ++i) {
        auto *outputData = outputTensor[i].GetTensorMutableData<float>();
        if (i == 0 || i == 1 || i == 2 || i == 3) {
            outputDataVec.emplace_back(outputData, outputData + 1);
        }
        if (i == 4) {
            outputDataVec.emplace_back(outputData, outputData + 1 * 3);
        }
        if (i == 5 || i == 6) {
            outputDataVec.emplace_back(outputData, outputData + 1 * 21 * 3);
        }
    }

    return outputDataVec;
}

LivePortrait::Generator::Generator(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}

cv::Mat LivePortrait::Generator::generateFrame(std::vector<float> &featureVolume,
                                               std::vector<float> &sourceMotionPoints,
                                               std::vector<float> &targetMotionPoints) const {
    std::vector<Ort::Value> inputTensors;
    const std::vector<int64_t> inputFeatureShape{1, 32, 16, 64, 64};
    const std::vector<int64_t> inputMotionShape{1, 21, 3};
    for (const auto &name : m_inputNames) {
        std::string nameStr(name);
        if (nameStr == "feature_volume") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, featureVolume.data(), featureVolume.size(), inputFeatureShape.data(), inputFeatureShape.size()));
        }
        if (nameStr == "source") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, sourceMotionPoints.data(), sourceMotionPoints.size(), inputMotionShape.data(), inputMotionShape.size()));
        }
        if (nameStr == "target") {
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memoryInfo, targetMotionPoints.data(), targetMotionPoints.size(), inputMotionShape.data(), inputMotionShape.size()));
        }
    }

    auto outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(), inputTensors.data(), inputTensors.size(), m_outputNames.data(), m_outputNames.size());

    auto *outputData = outputTensor.front().GetTensorMutableData<float>();
    std::vector<int64_t> outsShape = outputTensor[0].GetTensorTypeAndShapeInfo().GetShape();
    const long long outputHeight = outsShape[2];
    const long long outputWidth = outsShape[3];

    const long long channelStep = outputHeight * outputWidth;
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

std::vector<float> LivePortrait::getInputImageData(const cv::Mat &image, const cv::Size &size) {
    cv::Mat inputImage;
    cv::resize(image, inputImage, size, cv::InterpolationFlags::INTER_AREA);
    std::vector<cv::Mat> bgrChannels(3);
    cv::split(inputImage, bgrChannels);
    for (int i = 0; i < 3; ++i) {
        bgrChannels[i].convertTo(bgrChannels[i], CV_32FC1, 1.0 / 255.0);
    }

    const int imageArea = inputImage.cols * inputImage.rows;
    std::vector<float> inputImageData(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputImageData.data(), (float *)bgrChannels[2].data, singleChnSize);                 // R
    memcpy(inputImageData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);     // G
    memcpy(inputImageData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize); // B
    return inputImageData;
}

cv::Mat LivePortrait::createRotationMat(float pitch, float yaw, float roll) {
    // Convert degrees to radians
    pitch = pitch * CV_PI / 180.0;
    yaw = yaw * CV_PI / 180.0;
    roll = roll * CV_PI / 180.0;

    // Create rotation matrices for each axis
    const cv::Mat Rx = (cv::Mat_<float>(3, 3) << 1, 0, 0,
                        0, cos(pitch), -sin(pitch),
                        0, sin(pitch), cos(pitch));

    const cv::Mat Ry = (cv::Mat_<float>(3, 3) << cos(yaw), 0, sin(yaw),
                        0, 1, 0,
                        -sin(yaw), 0, cos(yaw));

    const cv::Mat Rz = (cv::Mat_<float>(3, 3) << cos(roll), -sin(roll), 0,
                        sin(roll), cos(roll), 0,
                        0, 0, 1);

    // Combine the rotation matrices: R = Rz * Ry * Rx
    cv::Mat R = Rz * Ry * Rx;

    return R;
}

cv::Mat LivePortrait::limitExpression(const cv::Mat &expression) {
    static std::vector<float> expressionMin{
        -2.88067125e-02f, -8.12731311e-02f, -1.70541159e-03f,
        -4.88598682e-02f, -3.32196616e-02f, -1.67431499e-04f,
        -6.75425082e-02f, -4.28681746e-02f, -1.98950816e-04f,
        -7.23103955e-02f, -3.28503326e-02f, -7.31324719e-04f,
        -3.87073644e-02f, -6.01546466e-02f, -5.50269964e-04f,
        -6.38048723e-02f, -2.23840728e-01f, -7.13261834e-04f,
        -3.02710701e-02f, -3.93195450e-02f, -8.24086510e-06f,
        -2.95799859e-02f, -5.39318882e-02f, -1.74219604e-04f,
        -2.92359516e-02f, -1.53050944e-02f, -6.30460854e-05f,
        -5.56493877e-03f, -2.34344602e-02f, -1.26858242e-04f,
        -4.37593013e-02f, -2.77768299e-02f, -2.70503685e-02f,
        -1.76926646e-02f, -1.91676542e-02f, -1.15090821e-04f,
        -8.34268332e-03f, -3.99775570e-03f, -3.27481248e-05f,
        -3.40162888e-02f, -2.81868968e-02f, -1.96679524e-04f,
        -2.91855410e-02f, -3.97511162e-02f, -2.81230678e-05f,
        -1.50395725e-02f, -2.49494594e-02f, -9.42573533e-05f,
        -1.67938769e-02f, -2.00953931e-02f, -4.00750607e-04f,
        -1.86435618e-02f, -2.48535164e-02f, -2.74416432e-02f,
        -4.61211195e-03f, -1.21660791e-02f, -2.93173041e-04f,
        -4.10017073e-02f, -7.43824020e-02f, -4.42762971e-02f,
        -1.90370996e-02f, -3.74363363e-02f, -1.34740388e-02f};
    static std::vector<float> expressionMax{
        4.46682945e-02f, 7.08772913e-02f, 4.08344204e-04f,
        2.14308221e-02f, 6.15894832e-02f, 4.85319615e-05f,
        3.02363783e-02f, 4.45043296e-02f, 1.28298725e-05f,
        3.05869691e-02f, 3.79812494e-02f, 6.57040102e-04f,
        4.45670523e-02f, 3.97259220e-02f, 7.10966764e-04f,
        9.43699256e-02f, 9.85926315e-02f, 2.02551950e-04f,
        1.61131397e-02f, 2.92906128e-02f, 3.44733417e-06f,
        5.23825921e-02f, 1.07065082e-01f, 6.61510974e-04f,
        2.85718683e-03f, 8.32320191e-03f, 2.39314613e-04f,
        2.57947259e-02f, 1.60935968e-02f, 2.41853559e-05f,
        4.90833223e-02f, 3.43903080e-02f, 3.22353356e-02f,
        1.44766076e-02f, 3.39248963e-02f, 1.42291479e-04f,
        8.75749043e-04f, 6.82212645e-03f, 2.76097053e-05f,
        1.86958015e-02f, 3.84016186e-02f, 7.33085908e-05f,
        2.01714113e-02f, 4.90544215e-02f, 2.34028921e-05f,
        2.46518422e-02f, 3.29151377e-02f, 3.48571630e-05f,
        2.22457591e-02f, 1.21796541e-02f, 1.56396593e-04f,
        1.72109623e-02f, 3.01626958e-02f, 1.36556877e-02f,
        1.83460284e-02f, 1.61141958e-02f, 2.87440169e-04f,
        3.57594155e-02f, 1.80554688e-01f, 2.75554154e-02f,
        2.17450950e-02f, 8.66811201e-02f, 3.34241726e-02f};

    cv::Mat limitedExpression;
    static cv::Mat expressionMinMat(21, 3, CV_32FC1, expressionMin.data());
    static cv::Mat expressionMaxMat(21, 3, CV_32FC1, expressionMax.data());
    cv::max(expression, expressionMinMat, limitedExpression);
    cv::min(limitedExpression, expressionMaxMat, limitedExpression);
    return limitedExpression;
}

} // namespace ffc::expressionRestore