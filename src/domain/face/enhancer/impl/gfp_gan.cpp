module;
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.enhancer;
import :gfp_gan;

namespace domain::face::enhancer {

GfpGan::GfpGan() {
    // Masker initialization deferred
}

void GfpGan::load_model(const std::string& model_path,
                        const foundation::ai::inference_session::Options& options) {
    FaceEnhancerImplBase::load_model(model_path, options);
    if (!m_session) return;

    auto input_dims = m_session->get_input_node_dims();
    if (!input_dims.empty() && input_dims[0].size() >= 4) {
        m_input_height = static_cast<int>(input_dims[0][2]);
        m_input_width = static_cast<int>(input_dims[0][3]);
        m_size = cv::Size(m_input_width, m_input_height);
    }

    m_input_names = {"input"};
    m_output_names = {"output"};
}

cv::Mat GfpGan::enhance_face(const cv::Mat& target_crop) {
    if (target_crop.empty()) return {};

    // Input Size Validation
    cv::Mat processed_crop = target_crop;
    if (processed_crop.size() != m_size) { cv::resize(processed_crop, processed_crop, m_size); }

    // foundation::infrastructure::logger::ScopedTimer timer("GfpGan::Inference");
    return apply_enhance(processed_crop);
}

std::tuple<std::vector<float>, std::vector<int64_t>> GfpGan::prepare_input(
    const cv::Mat& cropped_frame) const {
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

    return std::make_tuple(std::move(input_image_data), std::move(input_shape));
}

cv::Mat GfpGan::process_output(const std::vector<Ort::Value>& output_tensors) const {
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

cv::Mat GfpGan::apply_enhance(const cv::Mat& cropped_frame) const {
    auto [input_image_data, input_shape] = prepare_input(cropped_frame);

    std::vector<Ort::Value> input_tensors;
    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        m_memory_info.GetConst(), input_image_data.data(), input_image_data.size(),
        input_shape.data(), input_shape.size()));

    auto output_tensors = m_session->run(input_tensors);

    return process_output(output_tensors);
}

} // namespace domain::face::enhancer
