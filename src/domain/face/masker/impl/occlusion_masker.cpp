module;
#include <vector>
#include <iostream>
#include <algorithm>
#include <limits>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.masker;
import :impl_occlusion;
import foundation.ai.inference_session_registry;

namespace {

std::pair<std::vector<float>, std::vector<int64_t>> prepare_input(
    const cv::Mat& crop_vision_frame, const std::vector<std::vector<int64_t>>& input_node_dims) {
    if (input_node_dims.empty() || input_node_dims[0].size() < 3) { return {}; }

    int h = static_cast<int>(input_node_dims[0][1]);
    int w = static_cast<int>(input_node_dims[0][2]);

    // Fallback logic for dynamic shapes
    if (h <= 0) h = 256;
    if (w <= 0) w = 256;

    std::vector<int64_t> input_shape = {1, h, w, 3};

    // Resize
    cv::Mat resized;
    cv::resize(crop_vision_frame, resized, cv::Size(w, h));

    // BGR -> RGB & Normalize (1.0/255.0)
    cv::Mat float_img;
    cv::cvtColor(resized, float_img, cv::COLOR_BGR2RGB);
    float_img.convertTo(float_img, CV_32FC3, 1.0 / 255.0);

    // HWC Layout construction
    size_t input_tensor_size = 1 * h * w * 3;
    std::vector<float> input_data(input_tensor_size);

    if (float_img.isContinuous()) {
        std::memcpy(input_data.data(), float_img.data, input_tensor_size * sizeof(float));
    } else {
        cv::Mat cont = float_img.clone();
        std::memcpy(input_data.data(), cont.data, input_tensor_size * sizeof(float));
    }

    return {std::move(input_data), std::move(input_shape)};
}

cv::Mat process_output(std::vector<Ort::Value>& output_tensors, cv::Size original_size, int model_h,
                       int model_w) {
    auto& output_tensor = output_tensors[0];
    auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
    auto output_shape = type_info.GetShape();

    // Default to model input dims if output dims are weird, but usually [1, H, W, 1] or [1, H, W]
    int out_h = model_h;
    int out_w = model_w;
    if (output_shape.size() >= 3) {
        out_h = static_cast<int>(output_shape[1]);
        out_w = static_cast<int>(output_shape[2]);
    }

    float* output_buffer = output_tensor.GetTensorMutableData<float>();
    cv::Mat mask_float(out_h, out_w, CV_32FC1, output_buffer);

    // Soft thresholding / Clamping
    cv::Mat mask_clamped;
    cv::max(mask_float, 0.0, mask_clamped);
    cv::min(mask_clamped, 1.0, mask_clamped);

    // Resize to Original
    cv::Mat mask_resized;
    cv::resize(mask_clamped, mask_resized, original_size);

    // Blur
    cv::GaussianBlur(mask_resized, mask_resized, cv::Size(0, 0), 5);

    // Hard Threshold
    cv::Mat mask_binary;
    cv::threshold(mask_resized, mask_binary, 0.5, 255.0, cv::THRESH_BINARY);

    cv::Mat mask_u8;
    mask_binary.convertTo(mask_u8, CV_8UC1);

    return mask_u8;
}

} // namespace

namespace domain::face::masker {

void OcclusionMasker::load_model(const std::string& model_path,
                                 const foundation::ai::inference_session::Options& options) {
    m_session =
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->get_session(
            model_path, options);
}

cv::Mat OcclusionMasker::create_occlusion_mask(const cv::Mat& crop_vision_frame) {
    if (!m_session || !m_session->is_model_loaded() || crop_vision_frame.empty()) {
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    // 1. Prepare Input
    auto [input_data, input_shape] =
        prepare_input(crop_vision_frame, m_session->get_input_node_dims());
    if (input_data.empty()) { return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1); }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size()));

    // 2. Run Inference
    auto output_tensors = m_session->run(input_tensors);
    if (output_tensors.empty()) return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);

    // 3. Process Output
    // Need model dims for fallback in process_output
    int h = static_cast<int>(input_shape[1]);
    int w = static_cast<int>(input_shape[2]);
    return process_output(output_tensors, crop_vision_frame.size(), h, w);
}

} // namespace domain::face::masker
