module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <iostream>
#include <algorithm>

module domain.face.masker;
import :impl_occlusion;

namespace domain::face::masker {

cv::Mat OcclusionMasker::create_occlusion_mask(const cv::Mat& crop_vision_frame) {
    if (!is_model_loaded() || crop_vision_frame.empty()) {
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    // 1. Get input dimensions
    // Expected HWC: [1, H, W, 3] or similar
    auto input_dims = get_input_node_dims();
    if (input_dims.empty() || input_dims[0].size() < 4) {
        // Fallback or error
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    // Assuming [1, H, W, 3] for HWC based on requirement
    // But let's check if channels are last
    int h = 0, w = 0;
    std::vector<int64_t> input_shape = input_dims[0];

    // Heuristic: if last dim is 3, it's HWC. If 2nd dim is 3, it's NCHW.
    bool is_nchw = (input_shape[1] == 3);

    if (is_nchw) {
        h = static_cast<int>(input_shape[2]);
        w = static_cast<int>(input_shape[3]);
    } else {
        h = static_cast<int>(input_shape[1]);
        w = static_cast<int>(input_shape[2]);
    }

    // Pre-processing
    cv::Mat resized;
    cv::resize(crop_vision_frame, resized, cv::Size(w, h));

    // BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // Normalize [0, 1]
    cv::Mat float_img;
    rgb.convertTo(float_img, CV_32FC3, 1.0 / 255.0);

    // Prepare input data
    size_t input_tensor_size = 1 * h * w * 3;
    std::vector<float> input_data(input_tensor_size);

    if (is_nchw) {
        // HWC -> NCHW
        // Split channels
        std::vector<cv::Mat> channels(3);
        cv::split(float_img, channels);

        size_t channel_size = h * w;
        for (int c = 0; c < 3; ++c) {
            // Check continuity
            if (channels[c].isContinuous()) {
                std::memcpy(input_data.data() + c * channel_size, channels[c].data,
                            channel_size * sizeof(float));
            } else {
                cv::Mat cont = channels[c].clone();
                std::memcpy(input_data.data() + c * channel_size, cont.data,
                            channel_size * sizeof(float));
            }
        }
    } else {
        // HWC (Direct copy)
        if (float_img.isContinuous()) {
            std::memcpy(input_data.data(), float_img.data, input_tensor_size * sizeof(float));
        } else {
            cv::Mat cont = float_img.clone();
            std::memcpy(input_data.data(), cont.data, input_tensor_size * sizeof(float));
        }
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size()));

    // Run inference
    auto output_tensors = run(input_tensors);

    if (output_tensors.empty()) { return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1); }

    // Post-processing
    auto& output_tensor = output_tensors[0];
    auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
    auto output_shape = type_info.GetShape(); // [1, H, W, 1] or [1, H, W]

    float* output_buffer = output_tensor.GetTensorMutableData<float>();

    // Determine output H, W
    // If [1, H, W, 1] -> dims=4
    // If [1, H, W] -> dims=3
    int out_h = 0, out_w = 0;
    if (output_shape.size() == 4) {
        out_h = static_cast<int>(output_shape[1]);
        out_w = static_cast<int>(output_shape[2]);
    } else if (output_shape.size() == 3) {
        out_h = static_cast<int>(output_shape[1]);
        out_w = static_cast<int>(output_shape[2]);
    } else {
        // Fallback
        out_h = h;
        out_w = w;
    }

    cv::Mat mask_float(out_h, out_w, CV_32FC1, output_buffer);

    // Threshold > 0.5 -> 255
    cv::Mat mask_binary;
    cv::threshold(mask_float, mask_binary, 0.5, 255.0, cv::THRESH_BINARY);

    cv::Mat mask_u8;
    mask_binary.convertTo(mask_u8, CV_8UC1);

    // Resize to original
    if (mask_u8.size() != crop_vision_frame.size()) {
        cv::resize(mask_u8, mask_u8, crop_vision_frame.size(), 0, 0, cv::INTER_NEAREST);
    }

    return mask_u8;
}

} // namespace domain::face::masker
