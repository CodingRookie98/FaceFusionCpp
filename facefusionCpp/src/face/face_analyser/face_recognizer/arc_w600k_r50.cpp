/**
 ******************************************************************************
 * @file           : fr_arc_w_600_k_r_50.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_recognizer_hub;
import :arc_w600k_r50;
import face_helper;

namespace ffc::faceRecognizer {
ArcW600kR50::ArcW600kR50(const std::shared_ptr<Ort::Env> &env) :
    FaceRecognizerBase(env) {
}

void ArcW600kR50::LoadModel(const std::string &modelPath, const Options &options) {
    FaceRecognizerBase::LoadModel(modelPath, options);
    m_inputWidth = static_cast<int>(input_node_dims_[0][2]);
    m_inputHeight = static_cast<int>(input_node_dims_[0][3]);
}

std::vector<float> ArcW600kR50::preProcess(const cv::Mat &visionFrame, const Face::Landmarks &faceLandmark5_68) const {
    std::vector<cv::Point2f> warpTemplate = face_helper::getWarpTemplate(face_helper::WarpTemplateType::Arcface_112_v2);
    cv::Mat cropVisionFrame;
    std::tie(cropVisionFrame, std::ignore) = face_helper::warpFaceByFaceLandmarks5(visionFrame, faceLandmark5_68,
                                                                                  warpTemplate,
                                                                                  cv::Size(112, 112));
    std::vector<cv::Mat> bgrChannels(3);
    split(cropVisionFrame, bgrChannels);
    for (int c = 0; c < 3; c++) {
        bgrChannels[c].convertTo(bgrChannels[c], CV_32FC1, 1 / 127.5, -1.0);
    }

    const int imageArea = this->m_inputHeight * this->m_inputWidth;
    std::vector<float> inputData;
    inputData.resize(3 * imageArea);
    size_t singleChnSize = imageArea * sizeof(float);
    memcpy(inputData.data(), (float *)bgrChannels[2].data, singleChnSize);
    memcpy(inputData.data() + imageArea, (float *)bgrChannels[1].data, singleChnSize);
    memcpy(inputData.data() + imageArea * 2, (float *)bgrChannels[0].data, singleChnSize);
    return inputData;
}

std::array<std::vector<float>, 2> ArcW600kR50::recognize(const cv::Mat &visionFrame, const Face::Landmarks &faceLandmark5) {
    std::vector<float> inputData = this->preProcess(visionFrame, faceLandmark5);
    const std::vector<int64_t> inputImgShape{1, 3, this->m_inputHeight, this->m_inputWidth};
    const Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memory_info_->GetConst(), inputData.data(),
                                                                   inputData.size(), inputImgShape.data(),
                                                                   inputImgShape.size());

    std::vector<Ort::Value> ortOutputs = ort_session_->Run(run_options_, input_names_.data(),
                                                           &inputTensor, 1, output_names_.data(),
                                                           output_names_.size());

    auto *pdata = ortOutputs[0].GetTensorMutableData<float>(); /// 形状是(1, 512)
    const int lenFeature = ortOutputs[0].GetTensorTypeAndShapeInfo().GetShape()[1];

    Face::Embeddings embedding(lenFeature), normedEmbedding(lenFeature);

    memcpy(embedding.data(), pdata, lenFeature * sizeof(float));

    const double norm = cv::norm(embedding, cv::NORM_L2);
    for (int i = 0; i < lenFeature; i++) {
        normedEmbedding.at(i) = embedding.at(i) / (float)norm;
    }

    return {embedding, normedEmbedding};
}
}