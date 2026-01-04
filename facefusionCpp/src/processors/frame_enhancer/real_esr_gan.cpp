/**
 ******************************************************************************
 * @file           : real_esr_gan.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-30
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

module frame_enhancer;
import :real_esr_gan;
import vision;

namespace ffc::frame_enhancer {

using namespace media;

RealEsrGan::RealEsrGan(const std::shared_ptr<Ort::Env>& env) :
    InferenceSession(env) {
}

std::string RealEsrGan::get_processor_name() const {
    return "FrameEnhancer.RealEsrGan";
}

cv::Mat RealEsrGan::enhanceFrame(const RealEsrGanInput& input) const {
    if (input.target_frame == nullptr) {
        return {};
    }
    if (input.target_frame->empty()) {
        return {};
    }
    const int &tempWidth = input.target_frame->cols, &tempHeight = input.target_frame->rows;
    auto [tileVisionFrames, padWidth, pidHeight] = vision::create_tile_frames(*input.target_frame, m_tileSize);

    for (size_t i = 0; i < tileVisionFrames.size(); i++) {
        std::vector<float> inputImageData = get_input_data(tileVisionFrames[i]);
        std::vector<int64_t> inputShape{1, 3, tileVisionFrames[i].cols, tileVisionFrames[i].rows};
        std::vector<Ort::Value> inputTensors;
        inputTensors.emplace_back(Ort::Value::CreateTensor<float>(m_memory_info->GetConst(), inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

        std::vector<Ort::Value> outputTensor = m_ort_session->Run(m_run_options, m_input_names.data(),
                                                                  inputTensors.data(), inputTensors.size(),
                                                                  m_output_names.data(), m_output_names.size());
        const float* outputData = outputTensor[0].GetTensorMutableData<float>();
        const int outputWidth = static_cast<int>(outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[2]);
        const int outputHeight = static_cast<int>(outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[3]);
        cv::Mat outputImage = get_output_data(outputData, cv::Size(outputWidth, outputHeight));
        tileVisionFrames[i] = std::move(outputImage);
    }

    cv::Mat outputImage = vision::merge_tile_frames(tileVisionFrames, tempWidth * m_modelScale, tempHeight * m_modelScale,
                                                    padWidth * m_modelScale, pidHeight * m_modelScale,
                                                    {m_tileSize[0] * m_modelScale, m_tileSize[1] * m_modelScale, m_tileSize[2] * m_modelScale});
    if (input.blend > 100) {
        outputImage = blend_frame(*input.target_frame, outputImage, 100);
    } else {
        outputImage = blend_frame(*input.target_frame, outputImage, input.blend);
    }
    return outputImage;
}
} // namespace ffc::frame_enhancer