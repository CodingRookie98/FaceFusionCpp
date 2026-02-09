module;
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.masker;
import :impl_region;
import foundation.ai.inference_session_registry;

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

namespace {

std::pair<std::vector<float>, std::vector<int64_t>> prepare_region_input(
    const cv::Mat& crop_vision_frame, const std::vector<std::vector<int64_t>>& input_node_dims) {
    if (input_node_dims.empty()) return {};

    int h = 512;
    int w = 512;

    if (input_node_dims[0].size() >= 4) {
        h = static_cast<int>(input_node_dims[0][2]);
        w = static_cast<int>(input_node_dims[0][3]);
    }

    cv::Mat resized;
    cv::resize(crop_vision_frame, resized, cv::Size(w, h));
    cv::flip(resized, resized, 1);

    cv::Mat float_img;
    cv::cvtColor(resized, float_img, cv::COLOR_BGR2RGB);
    float_img.convertTo(float_img, CV_32FC3, 1.0 / 127.5, -1.0);

    // NCHW Layout construction
    size_t channel_size = h * w;
    std::vector<float> input_data(1 * 3 * channel_size);
    std::vector<cv::Mat> channels(3);
    cv::split(float_img, channels);

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
    return {std::move(input_data), std::move(input_shape)};
}

cv::Mat process_region_output(std::vector<Ort::Value>& output_tensors, cv::Size original_size,
                              const std::unordered_set<FaceRegion>& regions) {
    const auto& output_tensor = output_tensors[0];
    auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
    auto output_shape = type_info.GetShape();

    if (output_shape.size() != 4) { return cv::Mat::zeros(original_size, CV_8UC1); }

    int num_classes = static_cast<int>(output_shape[1]);
    int out_h = static_cast<int>(output_shape[2]);
    int out_w = static_cast<int>(output_shape[3]);

    const float* output_data = output_tensor.GetTensorData<float>();

    std::unordered_set<int> target_indices;
    for (auto region : regions) {
        int id = get_region_id(region);
        if (id != -1) target_indices.insert(id);
    }

    cv::Mat mask = cv::Mat::zeros(out_h, out_w, CV_8UC1);
    int pixels = out_h * out_w;

    for (int i = 0; i < pixels; ++i) {
        int best_class = 0;
        float max_val = -std::numeric_limits<float>::infinity();

        for (int c = 0; c < num_classes; ++c) {
            float val = output_data[c * pixels + i];
            if (val > max_val) {
                max_val = val;
                best_class = c;
            }
        }

        if (target_indices.contains(best_class)) { mask.data[i] = 255; }
    }

    cv::flip(mask, mask, 1);
    if (mask.size() != original_size) {
        cv::resize(mask, mask, original_size, 0, 0, cv::INTER_NEAREST);
    }

    return mask;
}

} // namespace

void RegionMasker::load_model(const std::string& model_path,
                              const foundation::ai::inference_session::Options& options) {
    m_session =
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->get_session(
            model_path, options);
}

cv::Mat RegionMasker::create_region_mask(const cv::Mat& crop_vision_frame,
                                         const std::unordered_set<FaceRegion>& regions) {
    if (!m_session || !m_session->is_model_loaded() || crop_vision_frame.empty()) {
        return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);
    }

    // 1. Prepare Input
    auto [input_data, input_shape] =
        prepare_region_input(crop_vision_frame, m_session->get_input_node_dims());
    if (input_data.empty()) { return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1); }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size()));

    // 2. Run Inference
    auto output_tensors = m_session->run(input_tensors);
    if (output_tensors.empty()) return cv::Mat::zeros(crop_vision_frame.size(), CV_8UC1);

    // 3. Process Output
    return process_region_output(output_tensors, crop_vision_frame.size(), regions);
}

} // namespace domain::face::masker
