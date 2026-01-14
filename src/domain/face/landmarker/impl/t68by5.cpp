module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <tuple>

module domain.face.landmarker;
import :impl;
import domain.face.helper;
import foundation.ai.inference_session;

namespace domain::face::landmarker {

struct T68By5::Impl {
    foundation::ai::inference_session::InferenceSession session;
    int input_height{0};
    int input_width{0};

    std::tuple<std::vector<float>, cv::Mat> pre_process(
        const domain::face::types::Landmarks& landmarks5) const {
        auto landmark5 = landmarks5;
        const auto warp_template = domain::face::helper::get_warp_template(
            domain::face::helper::WarpTemplateType::Ffhq_512);

        cv::Mat affine_matrix = domain::face::helper::estimate_matrix_by_face_landmark_5(
            landmark5, warp_template, cv::Size(1, 1));

        cv::transform(landmark5, landmark5, affine_matrix);

        std::vector<float> tensor_data;
        tensor_data.reserve(landmark5.size() * 2);
        for (const auto& point : landmark5) {
            tensor_data.emplace_back(point.x);
            tensor_data.emplace_back(point.y);
        }
        return {tensor_data, affine_matrix};
    }
};

T68By5::T68By5() : p_impl(std::make_unique<Impl>()) {}
T68By5::~T68By5() = default;

void T68By5::load_model(const std::string& model_path,
                        const foundation::ai::inference_session::Options& options) {
    p_impl->session.load_model(model_path, options);
    auto input_dims = p_impl->session.get_input_node_dims();
    if (!input_dims.empty()) {
        p_impl->input_height = static_cast<int>(input_dims[0][1]);
        p_impl->input_width = static_cast<int>(input_dims[0][2]);
    }
}

LandmarkerResult T68By5::detect(const cv::Mat& image, const cv::Rect2f& bbox) {
    // 68By5 不支持直接从图像检测，它需要 5点关键点
    return {};
}

domain::face::types::Landmarks T68By5::expand_68_from_5(
    const domain::face::types::Landmarks& landmarks5) {
    if (!p_impl->session.is_model_loaded() || landmarks5.empty()) { return {}; }

    auto [input_data, affine_matrix] = p_impl->pre_process(landmarks5);
    const std::vector<int64_t> input_shape{1, p_impl->input_height, p_impl->input_width};

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

    std::vector<Ort::Value> inputs;
    inputs.push_back(std::move(input_tensor));

    auto outputs = p_impl->session.run(inputs);
    if (outputs.empty()) { return {}; }

    const float* p_data = outputs[0].GetTensorMutableData<float>(); // shape(1, 68, 2)
    const auto shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
    const int num_points = static_cast<int>(shape[1]);

    domain::face::types::Landmarks landmarks68;
    landmarks68.reserve(num_points);
    for (int i = 0; i < num_points; ++i) {
        landmarks68.emplace_back(p_data[i * 2], p_data[i * 2 + 1]);
    }

    cv::Mat affine_matrix_inv;
    cv::invertAffineTransform(affine_matrix, affine_matrix_inv);
    cv::transform(landmarks68, landmarks68, affine_matrix_inv);

    return landmarks68;
}

} // namespace domain::face::landmarker
