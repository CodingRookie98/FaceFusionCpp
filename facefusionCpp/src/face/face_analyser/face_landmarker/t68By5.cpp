/**
 ******************************************************************************
 * @file           : face_landmarker_68_5.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_landmarker_hub;
import :t68by5;
import face;
import face_helper;

namespace ffc::faceLandmarker {
T68By5::T68By5(const std::shared_ptr<Ort::Env> &env) :
    FaceLandmarkerBase(env) {
}

void T68By5::loadModel(const std::string &modelPath, const Options &options) {
    FaceLandmarkerBase::loadModel(modelPath, options);
    m_inputHeight = m_inputNodeDims[0][1];
    m_inputWidth = m_inputNodeDims[0][2];
}

std::tuple<std::vector<float>, cv::Mat> T68By5::preProcess(const Face::Landmark &faceLandmark5) {
    Face::Landmark landmark5 = faceLandmark5;
    const std::vector<cv::Point2f> warpTemplate = FaceHelper::getWarpTemplate(FaceHelper::WarpTemplateType::Ffhq_512);

    cv::Mat affineMatrix = FaceHelper::estimateMatrixByFaceLandmark5(landmark5, warpTemplate, cv::Size(1, 1));
    cv::transform(landmark5, landmark5, affineMatrix);

    std::vector<float> tensorData;
    for (const auto &point : landmark5) {
        tensorData.emplace_back(point.x);
        tensorData.emplace_back(point.y);
    }
    return std::make_tuple(tensorData, affineMatrix);
}

Face::Landmark T68By5::detect(const Face::Landmark &faceLandmark5) const {
    std::vector<float> inputTensorData;
    cv::Mat affineMatrix;
    std::tie(inputTensorData, affineMatrix) = this->preProcess(faceLandmark5);

    std::vector<int64_t> inputShape{1, this->m_inputHeight, this->m_inputWidth};
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(m_memoryInfo,
                                                             inputTensorData.data(),
                                                             inputTensorData.size(), inputShape.data(),
                                                             inputShape.size());
    std::vector<Ort::Value> outputTensor = m_ortSession->Run(m_runOptions, m_inputNames.data(),
                                                             &inputTensor, 1, m_outputNames.data(),
                                                             m_outputNames.size());
    auto *pData = outputTensor[0].GetTensorMutableData<float>(); // shape(1, 68, 2);
    Face::Landmark faceLandMark68_5;
    for (int i = 0; i < 68; ++i) {
        faceLandMark68_5.emplace_back(pData[i * 2], pData[i * 2 + 1]);
    }

    // 将result转换为Mat类型，并确保形状为 (68, 2)
    cv::Mat resultMat(faceLandMark68_5);

    // 进行仿射变换
    cv::Mat transformedMat;
    cv::Mat affineMatrixInv;
    cv::invertAffineTransform(affineMatrix, affineMatrixInv);
    cv::transform(resultMat, transformedMat, affineMatrixInv);

    // 更新result
    faceLandMark68_5.clear();
    for (int i = 0; i < transformedMat.rows; ++i) {
        faceLandMark68_5.emplace_back(transformedMat.at<cv::Point2f>(i));
    }
    return faceLandMark68_5;
}
}