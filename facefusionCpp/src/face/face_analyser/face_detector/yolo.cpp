/**
 ******************************************************************************
 * @file           : face_yolov_8.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-4
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <common_macros.h>

module face_detector_hub;
import :yolo;
import face;
import vision;

namespace ffc::faceDetector {

Yolo::Yolo(const std::shared_ptr<Ort::Env> &env) :
    FaceDetectorBase(env) {
}

void Yolo::loadModel(const std::string &modelPath, const Options &options) {
    FaceDetectorBase::loadModel(modelPath, options);
    m_inputHeight = m_inputNodeDims[0][2];
    m_inputWidth = m_inputNodeDims[0][3];
}

std::tuple<std::vector<float>, float, float>
Yolo::preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize) {
    const int faceDetectorHeight = faceDetectorSize.height;
    const int faceDetectorWidth = faceDetectorSize.width;

    cv::Mat tempVisionFrame = Vision::resizeFrame(visionFrame, faceDetectorSize);
    float ratioHeight = static_cast<float>(visionFrame.rows) / static_cast<float>(tempVisionFrame.rows);
    float ratioWidth = static_cast<float>(visionFrame.cols) / static_cast<float>(tempVisionFrame.cols);

    // 创建一个指定尺寸的全零矩阵
    cv::Mat detectVisionFrame = cv::Mat::zeros(faceDetectorHeight, faceDetectorWidth, CV_32FC3);
    // 将输入的图像帧复制到全零矩阵的左上角
    tempVisionFrame.copyTo(detectVisionFrame(cv::Rect(0, 0, tempVisionFrame.cols, tempVisionFrame.rows)));

    std::vector<cv::Mat> bgrChannels(3);
    cv::split(detectVisionFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / 128.0, -127.5 / 128.0);
    }

    const int imageArea = faceDetectorWidth * faceDetectorHeight;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    const size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float *)bgrChannels[0].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float *)bgrChannels[2].data, singleChnSize);
    return std::make_tuple(inputData, ratioHeight, ratioWidth);
}

Yolo::Result
Yolo::detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                  const float &scoreThreshold) {
    auto [inputData, ratioHeight, ratioWidth] = this->preProcess(visionFrame, faceDetectorSize);

    const std::vector<int64_t> inputImgShape = {1, 3, faceDetectorSize.height, faceDetectorSize.width};
    const Ort::Value inputTensor = Ort::Value::CreateTensor<float>(m_memoryInfo, inputData.data(), inputData.size(), inputImgShape.data(), inputImgShape.size());

    std::vector<Ort::Value> ortOutputs = m_ortSession->Run(m_runOptions, m_inputNames.data(),
                                                           &inputTensor, 1, m_outputNames.data(),
                                                           m_outputNames.size());

    // 不需要手动释放 pdata，它由 Ort::Value 管理
    float *pdata = ortOutputs[0].GetTensorMutableData<float>(); /// 形状是(1, 20, 8400),不考虑第0维batchsize，每一列的长度20,前4个元素是检测框坐标(cx,cy,w,h)，第4个元素是置信度，剩下的15个元素是5个关键点坐标x,y和置信度
    const int numBox = ortOutputs[0].GetTensorTypeAndShapeInfo().GetShape()[2];

    std::vector<Face::BBox> bBoxRaw;
    std::vector<float> scoreRaw;
    std::vector<Face::Landmark> landmarkRaw;
    for (int i = 0; i < numBox; i++) {
        const float score = pdata[4 * numBox + i];
        if (score > scoreThreshold) {
            float xmin = (pdata[i] - 0.5 * pdata[2 * numBox + i]) * ratioWidth;           ///(cx,cy,w,h)转到(x,y,w,h)并还原到原图
            float ymin = (pdata[numBox + i] - 0.5 * pdata[3 * numBox + i]) * ratioHeight; ///(cx,cy,w,h)转到(x,y,w,h)并还原到原图
            float xmax = (pdata[i] + 0.5 * pdata[2 * numBox + i]) * ratioWidth;           ///(cx,cy,w,h)转到(x,y,w,h)并还原到原图
            float ymax = (pdata[numBox + i] + 0.5 * pdata[3 * numBox + i]) * ratioHeight; ///(cx,cy,w,h)转到(x,y,w,h)并还原到原图

            // 坐标的越界检查保护
            xmin = std::max(0.0f, std::min(xmin, static_cast<float>(visionFrame.cols)));
            ymin = std::max(0.0f, std::min(ymin, static_cast<float>(visionFrame.rows)));
            xmax = std::max(0.0f, std::min(xmax, static_cast<float>(visionFrame.cols)));
            ymax = std::max(0.0f, std::min(ymax, static_cast<float>(visionFrame.rows)));

            bBoxRaw.emplace_back(Face::BBox{xmin, ymin, xmax, ymax});
            scoreRaw.emplace_back(score);

            // 剩下的5个关键点坐标的计算
            Face::Landmark faceLandmark;
            for (int j = 5; j < 20; j += 3) {
                cv::Point2f point2F;
                point2F.x = pdata[j * numBox + i] * ratioWidth;
                point2F.y = pdata[(j + 1) * numBox + i] * ratioHeight;
                faceLandmark.emplace_back(point2F);
            }
            landmarkRaw.emplace_back(faceLandmark);
        }
    }

    Result result;
    result.bboxes = std::move(bBoxRaw);
    result.landmarks = std::move(landmarkRaw);
    result.scores = std::move(scoreRaw);

    return result;
}
}