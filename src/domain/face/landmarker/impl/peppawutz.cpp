module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <tuple>
#include <algorithm>

module domain.face.landmarker;
import :impl;
import domain.face.helper;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

namespace domain::face::landmarker {

struct Peppawutz::Impl {
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> session;
    int input_height{0};
    int input_width{0};
    cv::Size input_size{256, 256};

    std::tuple<std::vector<float>, cv::Mat> pre_process(const cv::Mat& vision_frame,
                                                        const cv::Rect2f& bbox) const {
        float sub_max = std::max(bbox.width, bbox.height);
        sub_max = std::max(sub_max, 1.f);
        const float scale = 195.f / sub_max;
        const std::vector<float> translation{
            (static_cast<float>(input_size.width) - (bbox.x * 2.0f + bbox.width) * scale) * 0.5f,
            (static_cast<float>(input_size.width) - (bbox.y * 2.0f + bbox.height) * scale) * 0.5f};

        auto [crop_img, affine_matrix] = domain::face::helper::warp_face_by_translation(
            vision_frame, translation, scale, input_size);

        crop_img = domain::face::helper::conditional_optimize_contrast(crop_img);

        cv::Mat inv_affine_matrix;
        cv::invertAffineTransform(affine_matrix, inv_affine_matrix);

        std::vector<cv::Mat> bgr_channels(3);
        cv::split(crop_img, bgr_channels);
        for (int c = 0; c < 3; c++) {
            // 回到 [0, 1] 归一化
            bgr_channels[c].convertTo(bgr_channels[c], CV_32FC1, 1.0 / 255.0);
        }

        const int image_area = input_height * input_width;
        std::vector<float> input_data(3 * image_area);
        const size_t single_chn_size = image_area * sizeof(float);

        // BGR 顺序 (与旧代码一致)
        std::memcpy(input_data.data(), bgr_channels[0].data, single_chn_size);
        std::memcpy(input_data.data() + image_area, bgr_channels[1].data, single_chn_size);
        std::memcpy(input_data.data() + image_area * 2, bgr_channels[2].data, single_chn_size);

        return {input_data, inv_affine_matrix};
    }
};

Peppawutz::Peppawutz() : p_impl(std::make_unique<Impl>()) {}
Peppawutz::~Peppawutz() = default;

void Peppawutz::load_model(const std::string& model_path,
                           const foundation::ai::inference_session::Options& options) {
    p_impl->session =
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->get_session(
            model_path, options);
    auto input_dims = p_impl->session->get_input_node_dims();
    if (!input_dims.empty() && input_dims[0].size() >= 4) {
        p_impl->input_height = static_cast<int>(input_dims[0][2]);
        p_impl->input_width = static_cast<int>(input_dims[0][3]);

        // 处理动态维度，默认为 256
        if (p_impl->input_height <= 0) p_impl->input_height = 256;
        if (p_impl->input_width <= 0) p_impl->input_width = 256;

        p_impl->input_size = cv::Size(p_impl->input_width, p_impl->input_height);
    } else {
        // 后备方案
        p_impl->input_height = 256;
        p_impl->input_width = 256;
        p_impl->input_size = cv::Size(256, 256);
    }
}

LandmarkerResult Peppawutz::detect(const cv::Mat& image, const cv::Rect2f& bbox) {
    if (!p_impl->session || !p_impl->session->is_model_loaded()) { return {}; }

    auto [input_data, inv_affine_matrix] = p_impl->pre_process(image, bbox);
    const std::vector<int64_t> input_shape{1, 3, p_impl->input_height, p_impl->input_width};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

    std::vector<Ort::Value> inputs;
    inputs.push_back(std::move(input_tensor));

    auto outputs = p_impl->session->run(inputs);
    if (outputs.empty()) { return {}; }

    const float* landmark_data = outputs[0].GetTensorMutableData<float>();
    const auto shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    const int num_points = static_cast<int>(shape[1]);

    std::vector<cv::Point2f> points(num_points);
    std::vector<float> scores(num_points);
    for (int i = 0; i < num_points; ++i) {
        const float x = landmark_data[i * 3] / 64.0f * static_cast<float>(p_impl->input_width);
        const float y = landmark_data[i * 3 + 1] / 64.0f
                      * static_cast<float>(p_impl->input_width); // 使用 width
        const float score = landmark_data[i * 3 + 2];
        points[i] = cv::Point2f(x, y);
        scores[i] = score;
    }

    cv::transform(points, points, inv_affine_matrix);

    float sum_score = 0.0f;
    for (float s : scores) sum_score += s;
    float mean_score = sum_score / static_cast<float>(num_points);

    // Peppawutz 映射分值范围不同 (0, 0.95 -> 0, 1)
    mean_score = domain::face::helper::interp({mean_score}, {0.0f, 0.95f}, {0.0f, 1.0f}).front();

    return {points, mean_score};
}

} // namespace domain::face::landmarker
