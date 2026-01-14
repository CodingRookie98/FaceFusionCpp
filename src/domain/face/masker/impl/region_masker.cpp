module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <iostream>

module domain.face.masker;
import :impl_region;

namespace domain::face::masker {

// Helper to map FaceRegion to Class ID (Assuming BiSeNet standard)
static int get_region_id(FaceRegion region) {
    switch (region) {
    case FaceRegion::Background: return 0;
    case FaceRegion::Skin: return 1;
    case FaceRegion::Nose: return 10;
    case FaceRegion::EyeGlasses: return 6;
    case FaceRegion::LeftEye: return 4;
    case FaceRegion::RightEye: return 5;
    case FaceRegion::LeftEyebrow: return 2;
    case FaceRegion::RightEyebrow: return 3;
    case FaceRegion::LeftEar: return 7;
    case FaceRegion::RightEar: return 8;
    case FaceRegion::Earring: return 9; // Often combined
    case FaceRegion::Mouth: return 11;
    case FaceRegion::UpperLip: return 12;
    case FaceRegion::LowerLip: return 13;
    case FaceRegion::Neck: return 14;
    case FaceRegion::Necklace: return 15; // Or Neck_L
    case FaceRegion::Cloth: return 16;
    case FaceRegion::Hair: return 17;
    case FaceRegion::Hat: return 18;
    default: return -1;
    }
}

cv::Mat RegionMasker::create_region_mask(const cv::Mat& crop_vision_frame,
                                         const std::unordered_set<FaceRegion>& regions) {
    if (!is_model_loaded() || crop_vision_frame.empty()) {
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    auto input_dims = get_input_node_dims();
    if (input_dims.empty()) return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);

    // Standard BiSeNet is 1x3x512x512 or similar
    int h = 512;
    int w = 512;

    // Attempt to read from dims
    if (input_dims[0].size() >= 4) {
        h = static_cast<int>(input_dims[0][2]);
        w = static_cast<int>(input_dims[0][3]);
    }

    // Pre-process
    cv::Mat resized;
    cv::resize(crop_vision_frame, resized, cv::Size(w, h));

    // Flip Horizontal (Requirement)
    cv::flip(resized, resized, 1);

    // BGR -> RGB
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // Normalize [-1, 1] => (x - 127.5) / 127.5
    cv::Mat float_img;
    rgb.convertTo(float_img, CV_32FC3, 1.0 / 127.5, -1.0);

    // NCHW Layout
    std::vector<float> input_data(1 * 3 * h * w);
    std::vector<cv::Mat> channels(3);
    cv::split(float_img, channels);

    size_t channel_size = h * w;
    for (int c = 0; c < 3; ++c) {
        if (channels[c].isContinuous()) {
            std::memcpy(input_data.data() + c * channel_size, channels[c].data,
                        channel_size * sizeof(float));
        } else {
            cv::Mat cont = channels[c].clone();
            std::memcpy(input_data.data() + c * channel_size, cont.data,
                        channel_size * sizeof(float));
        }
    }

    std::vector<int64_t> input_shape = {1, 3, h, w};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size()));

    // Run
    auto output_tensors = run(input_tensors);
    if (output_tensors.empty()) return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);

    // Post-process
    // Output should be [1, Classes, H, W] (Logits) or [1, H, W] (Indices)
    const auto& output_tensor = output_tensors[0];
    auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
    auto output_shape = type_info.GetShape();

    // Assuming logits [1, 19, H, W]
    if (output_shape.size() != 4) {
        // Fallback if shape is weird
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    int num_classes = static_cast<int>(output_shape[1]);
    int out_h = static_cast<int>(output_shape[2]);
    int out_w = static_cast<int>(output_shape[3]);

    const float* output_data = output_tensor.GetTensorData<float>();

    // We need to perform ArgMax per pixel
    // And map regions
    std::unordered_set<int> target_indices;
    for (auto region : regions) {
        int id = get_region_id(region);
        if (id != -1) target_indices.insert(id);
    }

    cv::Mat mask = cv::Mat::zeros(out_h, out_w, CV_8UC1);

    // ArgMax loop
    for (int i = 0; i < out_h * out_w; ++i) {
        int best_class = 0;
        float max_val = -std::numeric_limits<float>::infinity();

        for (int c = 0; c < num_classes; ++c) {
            float val = output_data[c * (out_h * out_w) + i];
            if (val > max_val) {
                max_val = val;
                best_class = c;
            }
        }

        if (target_indices.contains(best_class)) { mask.data[i] = 255; }
    }

    // Input was flipped, so Output is also flipped relative to original
    // Need to flip back
    cv::flip(mask, mask, 1);

    // Resize to original
    if (mask.size() != crop_vision_frame.size()) {
        cv::resize(mask, mask, crop_vision_frame.size(), 0, 0, cv::INTER_NEAREST);
    }

    return mask;
}

} // namespace domain::face::masker
