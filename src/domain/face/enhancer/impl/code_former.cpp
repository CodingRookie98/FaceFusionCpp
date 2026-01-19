module;
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.enhancer;
import :code_former;

namespace domain::face::enhancer {

CodeFormer::CodeFormer() {
    // Masker initialization deferred
}

void CodeFormer::load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options) {
    FaceEnhancerImplBase::load_model(model_path, options);
    if (!m_session) return;

    auto input_dims = m_session->get_input_node_dims();
    if (!input_dims.empty() && input_dims[0].size() >= 4) {
        m_input_height = static_cast<int>(input_dims[0][2]);
        m_input_width = static_cast<int>(input_dims[0][3]);
        m_size = cv::Size(m_input_width, m_input_height);
    }

    m_input_names = {"input", "weight"};
    m_output_names = {"output"};
}

cv::Mat CodeFormer::enhance_face(const EnhanceInput& input) {
    if (input.target_frame.empty()) { return {}; }
    if (input.target_faces_landmarks.empty()) { return input.target_frame.clone(); }

    if (!is_model_loaded()) { throw std::runtime_error("model is not loaded"); }

    std::vector<cv::Mat> cropped_target_frames;
    std::vector<cv::Mat> affine_matrices;
    std::vector<cv::Mat> cropped_result_frames;
    std::vector<cv::Mat> best_masks;

    for (const auto& landmarks : input.target_faces_landmarks) {
        auto [cropped_frame, affine_matrix] = domain::face::helper::warp_face_by_face_landmarks_5(
            input.target_frame, landmarks,
            domain::face::helper::get_warp_template(m_warp_template_type), m_size);
        cropped_target_frames.push_back(cropped_frame);
        affine_matrices.push_back(affine_matrix);
    }

    for (const auto& cropped_frame : cropped_target_frames) {
        cropped_result_frames.push_back(apply_enhance(cropped_frame));
    }

    for (auto& cropped_frame : cropped_target_frames) {
        cv::Mat mask = cv::Mat::ones(m_size, CV_32FC1);
        best_masks.push_back(mask);
    }

    cv::Mat result_frame = input.target_frame.clone();
    for (size_t i = 0; i < best_masks.size(); ++i) {
        result_frame = domain::face::helper::paste_back(result_frame, cropped_result_frames[i],
                                                        best_masks[i], affine_matrices[i]);
    }

    if (input.face_blend > 100) {
        result_frame = blend_frame(input.target_frame, result_frame, 100);
    } else {
        result_frame = blend_frame(input.target_frame, result_frame, input.face_blend);
    }
    return result_frame;
}

std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<double>, std::vector<int64_t>>
CodeFormer::prepare_input(const cv::Mat& cropped_frame) const {
    std::vector<cv::Mat> bgr_channels(3);
    cv::split(cropped_frame, bgr_channels);
    for (int c = 0; c < 3; c++) {
        bgr_channels[c].convertTo(bgr_channels[c], CV_32FC1, 1 / (255.0 * 0.5), -1.0);
    }

    const int image_area = cropped_frame.cols * cropped_frame.rows;
    std::vector<float> input_image_data(3 * image_area);
    const size_t single_chn_size = image_area * sizeof(float);
    memcpy(input_image_data.data(), bgr_channels[2].data, single_chn_size); // RGB order
    memcpy(input_image_data.data() + image_area, bgr_channels[1].data, single_chn_size);
    memcpy(input_image_data.data() + image_area * 2, bgr_channels[0].data, single_chn_size);

    std::vector<int64_t> input_shape{1, 3, m_input_height, m_input_width};
    std::vector<double> input_weight_data{1.0};
    std::vector<int64_t> weight_shape{1, 1};

    return std::make_tuple(std::move(input_image_data), std::move(input_shape),
                           std::move(input_weight_data), std::move(weight_shape));
}

cv::Mat CodeFormer::process_output(const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};

    const float* pdata = output_tensors[0].GetTensorData<float>();
    const auto outs_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int output_height = static_cast<int>(outs_shape[2]);
    const int output_width = static_cast<int>(outs_shape[3]);

    const long long channel_step = output_height * output_width;
    std::vector<cv::Mat> channel_mats(3);
    channel_mats[2] =
        cv::Mat(output_height, output_width, CV_32FC1, const_cast<float*>(pdata)).clone(); // R
    channel_mats[1] =
        cv::Mat(output_height, output_width, CV_32FC1, const_cast<float*>(pdata + channel_step))
            .clone(); // G
    channel_mats[0] =
        cv::Mat(output_height, output_width, CV_32FC1, const_cast<float*>(pdata + 2 * channel_step))
            .clone(); // B

    for (auto& mat : channel_mats) {
        cv::max(mat, -1.0f, mat);
        cv::min(mat, 1.0f, mat);
        mat = (mat + 1.0f) * 127.5f;
        cv::max(mat, 0.0f, mat);
        cv::min(mat, 255.0f, mat);
    }

    cv::Mat result_mat;
    cv::merge(channel_mats, result_mat);
    result_mat.convertTo(result_mat, CV_8UC3);
    return result_mat;
}

cv::Mat CodeFormer::apply_enhance(const cv::Mat& cropped_frame) const {
    auto [input_image_data, input_shape, input_weight_data, weight_shape] =
        prepare_input(cropped_frame);

    std::vector<Ort::Value> input_tensors;
    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        m_memory_info.GetConst(), input_image_data.data(), input_image_data.size(),
        input_shape.data(), input_shape.size()));

    input_tensors.emplace_back(Ort::Value::CreateTensor<double>(
        m_memory_info.GetConst(), input_weight_data.data(), input_weight_data.size(),
        weight_shape.data(), weight_shape.size()));

    auto output_tensors = m_session->run(input_tensors);

    return process_output(output_tensors);
}

} // namespace domain::face::enhancer
