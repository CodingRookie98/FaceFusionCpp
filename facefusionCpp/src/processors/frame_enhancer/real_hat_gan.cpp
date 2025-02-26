/**
 ******************************************************************************
 * @file           : real_hat_gan.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

module frame_enhancer;
import :real_hat_gan;
import vision;

namespace ffc::frameEnhancer {
RealHatGan::RealHatGan(const std::shared_ptr<Ort::Env>& env) :
    FrameEnhancerBase(), InferenceSession(env) {
}

std::string RealHatGan::getProcessorName() const {
    return "FrameEnhancer.RealHatGan";
}

cv::Mat RealHatGan::enhanceFrame(const RealHatGanInput& input) const {
    if (input.target_frame == nullptr) {
        return {};
    }
    if (input.target_frame->empty()) {
        return {};
    }
    const int tempWidth = input.target_frame->cols, tempHeight = input.target_frame->rows;
    auto [tileVisionFrames, padWidth, pidHeight] = ffc::vision::createTileFrames(*input.target_frame, m_tileSize);

    for (size_t i = 0; i < tileVisionFrames.size(); i++) {
        std::vector<float> inputImageData = getInputImageData(tileVisionFrames[i]);
        std::vector<int64_t> inputShape{1, 3, tileVisionFrames[i].cols, tileVisionFrames[i].rows};
        std::vector<Ort::Value> inputTensors;
        inputTensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info_->GetConst(), inputImageData.data(), inputImageData.size(), inputShape.data(), inputShape.size()));

        std::vector<Ort::Value> outputTensor = ort_session_->Run(run_options_, input_names_.data(),
                                                                 inputTensors.data(), inputTensors.size(),
                                                                 output_names_.data(), output_names_.size());
        const float* outputData = outputTensor[0].GetTensorMutableData<float>();
        const int outputWidth = static_cast<int>(outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[2]);
        const int outputHeight = static_cast<int>(outputTensor[0].GetTensorTypeAndShapeInfo().GetShape()[3]);
        cv::Mat outputImage = getOutputImage(outputData, cv::Size(outputWidth, outputHeight));
        tileVisionFrames[i] = std::move(outputImage);
    }

    cv::Mat outputImage = vision::mergeTileFrames(tileVisionFrames, tempWidth * m_modelScale, tempHeight * m_modelScale,
                                                  padWidth * m_modelScale, pidHeight * m_modelScale,
                                                  {m_tileSize[0] * m_modelScale, m_tileSize[1] * m_modelScale, m_tileSize[2] * m_modelScale});

    if (input.blend > 100) {
        outputImage = blendFrame(*input.target_frame, outputImage, 100);
    } else {
        outputImage = blendFrame(*input.target_frame, outputImage, input.blend);
    }
    return outputImage;
}
} // namespace ffc::frameEnhancer