/**
 ******************************************************************************
 * @file           : face_detector_retina.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-16
 ******************************************************************************
 */

#include "face_detector_retina.h"
#include "vision.h"
#include "face_helper.h"

FaceDetectorRetina::FaceDetectorRetina(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath) :
    FaceDetectorBase(env, modelPath) {
    m_inputHeight = m_inferenceSession.m_inputNodeDims[0][2];
    m_inputWidth = m_inferenceSession.m_inputNodeDims[0][3];
}

FaceDetectorRetina::Result FaceDetectorRetina::detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                                                      const float &scoreThreshold) {
    std::vector<float> inputData;
    float ratioHeight, ratioWidth;
    std::tie(inputData, ratioHeight, ratioWidth) = preProcess(visionFrame, faceDetectorSize);

    std::vector<int64_t> inputImgShape = {1, 3, faceDetectorSize.height, faceDetectorSize.width};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(m_inferenceSession.m_memoryInfo, inputData.data(), inputData.size(), inputImgShape.data(), inputImgShape.size());

    std::vector<Ort::Value> ortOutputs =
        m_inferenceSession.m_ortSession->Run(m_inferenceSession.m_runOptions,
                                             m_inferenceSession.m_inputNames.data(),
                                             &inputTensor, 1, m_inferenceSession.m_outputNames.data(),
                                             m_inferenceSession.m_outputNames.size());

    std::vector<Face::BBox> resultBoundingBoxes;
    std::vector<Face::Landmark> resultFaceLandmarks;
    std::vector<float> resultScores;

    for (size_t index = 0; index < m_featureStrides.size(); ++index) {
        int featureStride = m_featureStrides[index];
        std::vector<int> keepIndices;
        int size = ortOutputs[index].GetTensorTypeAndShapeInfo().GetShape()[0];
        float *pdataScoreRaw = ortOutputs[index].GetTensorMutableData<float>();
        for (size_t j = 0; j < size; ++j) {
            float tempScore = *(pdataScoreRaw + j);
            if (tempScore >= scoreThreshold) {
                keepIndices.emplace_back(j);
            }
        }

        if (keepIndices.empty()) {
            continue;
        }

        int strideHeight = std::floor(faceDetectorSize.height / featureStride);
        int strideWidth = std::floor(faceDetectorSize.width / featureStride);

        std::vector<std::array<int, 2>> anchors = FaceHelper::createStaticAnchors(m_featureStrides[index],
                                                                                  m_anchorTotal,
                                                                                  strideHeight,
                                                                                  strideWidth);

        std::vector<Face::BBox> boundingBoxesRaw;
        std::vector<Face::Landmark> faceLandmarksRaw;

        float *pdataBbox = ortOutputs[index + m_featureMapChannel].GetTensorMutableData<float>();
        float *pdataLandmark = ortOutputs[index + 2 * m_featureMapChannel].GetTensorMutableData<float>();
        float *pdataScore = ortOutputs[index].GetTensorMutableData<float>();

        size_t pdataBboxSize = ortOutputs[index + m_featureMapChannel].GetTensorTypeAndShapeInfo().GetShape()[0]
                               * ortOutputs[index + m_featureMapChannel].GetTensorTypeAndShapeInfo().GetShape()[1];
        for (size_t k = 0; k < pdataBboxSize; k += 4) {
            Face::BBox tempBbox;
            tempBbox.xmin = *(pdataBbox + k) * featureStride;
            tempBbox.ymin = *(pdataBbox + k + 1) * featureStride;
            tempBbox.xmax = *(pdataBbox + k + 2) * featureStride;
            tempBbox.ymax = *(pdataBbox + k + 3) * featureStride;
            boundingBoxesRaw.emplace_back(tempBbox);
        }

        size_t pdataLandmarkSize = ortOutputs[index + 2 * m_featureMapChannel].GetTensorTypeAndShapeInfo().GetShape()[0]
                                   * ortOutputs[index + 2 * m_featureMapChannel].GetTensorTypeAndShapeInfo().GetShape()[1];
        for (size_t k = 0; k < pdataLandmarkSize; k += 10) {
            Face::Landmark tempLandmark;
            tempLandmark.emplace_back(cv::Point2f(*(pdataLandmark + k), *(pdataLandmark + k + 1)));
            tempLandmark.emplace_back(cv::Point2f(*(pdataLandmark + k + 2), *(pdataLandmark + k + 3)));
            tempLandmark.emplace_back(cv::Point2f(*(pdataLandmark + k + 4), *(pdataLandmark + k + 5)));
            tempLandmark.emplace_back(cv::Point2f(*(pdataLandmark + k + 6), *(pdataLandmark + k + 7)));
            tempLandmark.emplace_back(cv::Point2f(*(pdataLandmark + k + 8), *(pdataLandmark + k + 9)));
            for (auto &point : tempLandmark) {
                point.x *= featureStride;
                point.y *= featureStride;
            }
            faceLandmarksRaw.emplace_back(tempLandmark);
        }

        for (const auto &keepIndex : keepIndices) {
            auto tempBbox = FaceHelper::distance2BBox(anchors[keepIndex], boundingBoxesRaw[keepIndex]);
            tempBbox.xmin *= ratioWidth;
            tempBbox.ymin *= ratioHeight;
            tempBbox.xmax *= ratioWidth;
            tempBbox.ymax *= ratioHeight;
            resultBoundingBoxes.emplace_back(tempBbox);

            auto tempLandmark = FaceHelper::distance2FaceLandmark5(anchors[keepIndex], faceLandmarksRaw[keepIndex]);
            for (auto &point : tempLandmark) {
                point.x *= ratioWidth;
                point.y *= ratioHeight;
            }
            resultFaceLandmarks.emplace_back(tempLandmark);

            resultScores.emplace_back(*(pdataScore + keepIndex));
        }
    }

    Result result;
    result.bboxes = std::move(resultBoundingBoxes);
    result.landmarks = std::move(resultFaceLandmarks);
    result.scores = std::move(resultScores);
    return result;
}

std::tuple<std::vector<float>, float, float>
FaceDetectorRetina::preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize) {
    const int faceDetectorHeight = faceDetectorSize.height;
    const int faceDetectorWidth = faceDetectorSize.width;

    auto tempVisionFrame = Ffc::Vision::resizeFrameResolution(visionFrame, cv::Size(faceDetectorWidth, faceDetectorHeight));
    float ratioHeight = (float)visionFrame.rows / (float)tempVisionFrame.rows;
    float ratioWidth = (float)visionFrame.cols / (float)tempVisionFrame.cols;

    // 创建一个指定尺寸的全零矩阵
    cv::Mat detectVisionFrame = cv::Mat::zeros(faceDetectorHeight, faceDetectorWidth, CV_32FC3);
    // 将输入的图像帧复制到全零矩阵的左上角
    tempVisionFrame.copyTo(detectVisionFrame(cv::Rect(0, 0, tempVisionFrame.cols, tempVisionFrame.rows)));

    std::vector<cv::Mat> bgrChannels(3);
    cv::split(detectVisionFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / 128.0, -127.5 / 128.0);
    }

    const int imageArea = faceDetectorHeight * faceDetectorWidth;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float *)bgrChannels[0].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float *)bgrChannels[2].data, singleChnSize);

    return std::make_tuple(inputData, ratioHeight, ratioWidth);
}