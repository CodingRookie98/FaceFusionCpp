module;
#include <algorithm>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>

module domain.frame.enhancer;

import :impl;
import foundation.media.vision;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

namespace domain::frame::enhancer {

FrameEnhancerImpl::FrameEnhancerImpl(const std::string& model_path,
                                     const foundation::ai::inference_session::Options& options,
                                     const std::vector<int>& tile_size, int model_scale) :
    m_tile_size(tile_size), m_model_scale(model_scale) {
    m_session = foundation::ai::inference_session::InferenceSessionRegistry::get_instance().get_session(
        model_path, options);
}

cv::Mat FrameEnhancerImpl::enhance_frame(const FrameEnhancerInput& input) const {
    if (input.target_frame.empty()) { return {}; }

    const int temp_width = input.target_frame.cols;
    const int temp_height = input.target_frame.rows;

    auto [tile_vision_frames, pad_width, pad_height] =
        foundation::media::vision::create_tile_frames(input.target_frame, m_tile_size);

    for (auto& tile_frame : tile_vision_frames) {
        std::vector<float> input_image_data = get_input_data(tile_frame);
        std::vector<int64_t> input_shape{1, 3, tile_frame.rows, tile_frame.cols};

        // Use memory info from Ort::MemoryInfo directly since wrapper doesn't expose it
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        std::vector<Ort::Value> input_tensors;
        input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
            memory_info, input_image_data.data(), input_image_data.size(), input_shape.data(),
            input_shape.size()));

        // Run
        std::vector<Ort::Value> output_tensors = m_session->run(input_tensors);

        if (output_tensors.empty()) continue;

        const float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();

        // shape is usually [1, 3, height, width]
        const int output_height = static_cast<int>(shape[2]);
        const int output_width = static_cast<int>(shape[3]);

        cv::Mat output_image = get_output_data(output_data, cv::Size(output_width, output_height));
        tile_frame = std::move(output_image);
    }

    cv::Mat output_image = foundation::media::vision::merge_tile_frames(
        tile_vision_frames, temp_width * m_model_scale, temp_height * m_model_scale,
        pad_width * m_model_scale, pad_height * m_model_scale,
        {m_tile_size[0] * m_model_scale, m_tile_size[1] * m_model_scale,
         m_tile_size[2] * m_model_scale});

    if (input.blend > 100) {
        output_image = blend_frame(input.target_frame, output_image, 100);
    } else {
        output_image = blend_frame(input.target_frame, output_image, input.blend);
    }

    return output_image;
}

cv::Mat FrameEnhancerImpl::blend_frame(const cv::Mat& temp_frame, const cv::Mat& merged_frame,
                                       int blend) {
    const float blend_factor = 1.0f - static_cast<float>(blend) / 100.0f;
    cv::Mat result;
    cv::resize(temp_frame, result, merged_frame.size());
    cv::addWeighted(result, blend_factor, merged_frame, 1.0f - blend_factor, 0, result);
    return result;
}

std::vector<float> FrameEnhancerImpl::get_input_data(const cv::Mat& frame) {
    std::vector<cv::Mat> bgr_channels(3);
    cv::split(frame, bgr_channels);
    for (int c = 0; c < 3; c++) {
        bgr_channels[c].convertTo(bgr_channels[c], CV_32FC1, 1.0 / 255.0);
    }

    const int image_area = frame.rows * frame.cols;
    std::vector<float> input_image_data(3 * image_area);
    const size_t single_chn_size = image_area * sizeof(float);
    memcpy(input_image_data.data(), (float*)bgr_channels[2].data, single_chn_size); // R
    memcpy(input_image_data.data() + image_area, (float*)bgr_channels[1].data,
           single_chn_size); // G
    memcpy(input_image_data.data() + image_area * 2, (float*)bgr_channels[0].data,
           single_chn_size); // B
    return input_image_data;
}

cv::Mat FrameEnhancerImpl::get_output_data(const float* output_data, const cv::Size& size) {
    const long long channel_step = size.width * size.height;
    cv::Mat output_image(size, CV_32FC3);

    auto* output_ptr = output_image.ptr<float>();
    // #pragma omp parallel for // Removed OMP for now, can add back if needed
    for (long long i = 0; i < channel_step; ++i) {
        // B, G, R order for OpenCV
        output_ptr[i * 3] =
            std::clamp(output_data[channel_step * 2 + i] * 255.0f, 0.0f, 255.0f); // B
        output_ptr[i * 3 + 1] =
            std::clamp(output_data[channel_step + i] * 255.0f, 0.0f, 255.0f);      // G
        output_ptr[i * 3 + 2] = std::clamp(output_data[i] * 255.0f, 0.0f, 255.0f); // R
    }

    return output_image;
}

} // namespace domain::frame::enhancer
