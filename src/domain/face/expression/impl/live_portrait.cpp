module;
#include <algorithm>
#include <cmath>
#include <format>
#include <future>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module domain.face.expression;

import :live_portrait;
import foundation.infrastructure.thread_pool;
import foundation.ai.inference_session_registry;

namespace domain::face::expression {

using namespace foundation::ai;
using namespace foundation::infrastructure::thread_pool;
using namespace foundation::ai::inference_session;

// FeatureExtractor Implementation
void LivePortrait::FeatureExtractor::load_model(const std::string& path,
                                                const inference_session::Options& options) {
    m_session = InferenceSessionRegistry::get_instance().get_session(path, options);
}

bool LivePortrait::FeatureExtractor::is_model_loaded() const {
    return m_session && m_session->is_model_loaded();
}

cv::Size LivePortrait::FeatureExtractor::get_input_size() const {
    if (!is_model_loaded()) return {256, 256};
    auto input_node_dims = m_session->get_input_node_dims();
    int width = static_cast<int>(input_node_dims[0][2]);
    int height = static_cast<int>(input_node_dims[0][3]);
    return {width, height};
}

std::tuple<std::vector<float>, std::vector<int64_t>> LivePortrait::FeatureExtractor::prepare_input(
    const cv::Mat& frame) const {
    auto input_node_dims = m_session->get_input_node_dims();
    const int width = static_cast<int>(input_node_dims[0][2]);
    const int height = static_cast<int>(input_node_dims[0][3]);

    std::vector<float> input_image_data =
        LivePortrait::get_input_image_data(frame, cv::Size(width, height));
    std::vector<int64_t> input_shape = {1, 3, width, height};

    return std::make_tuple(std::move(input_image_data), std::move(input_shape));
}

std::vector<float> LivePortrait::FeatureExtractor::process_output(
    const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};
    auto* output_data =
        output_tensors[0].GetTensorData<float>(); // Use GetTensorData for const correctness
    // Output size: 1 * 32 * 16 * 64 * 64
    constexpr size_t output_size = 1 * 32 * 16 * 64 * 64;
    return std::vector<float>(output_data, output_data + output_size);
}

std::vector<float> LivePortrait::FeatureExtractor::extract_feature(const cv::Mat& frame) const {
    if (!is_model_loaded()) { throw std::runtime_error("FeatureExtractor model is not loaded"); }

    auto [input_image_data, input_shape] = prepare_input(frame);

    // Create memory info locally
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<Ort::Value> input_tensors;
    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        memory_info, input_image_data.data(), input_image_data.size(), input_shape.data(),
        input_shape.size()));

    auto output_tensor = m_session->run(input_tensors);

    return process_output(output_tensor);
}

// MotionExtractor Implementation
void LivePortrait::MotionExtractor::load_model(const std::string& path,
                                               const inference_session::Options& options) {
    m_session = InferenceSessionRegistry::get_instance().get_session(path, options);
}

bool LivePortrait::MotionExtractor::is_model_loaded() const {
    return m_session && m_session->is_model_loaded();
}

std::tuple<std::vector<float>, std::vector<int64_t>> LivePortrait::MotionExtractor::prepare_input(
    const cv::Mat& frame) const {
    auto input_node_dims = m_session->get_input_node_dims();
    const int width = static_cast<int>(input_node_dims[0][2]);
    const int height = static_cast<int>(input_node_dims[0][3]);

    std::vector<float> input_image_data =
        LivePortrait::get_input_image_data(frame, cv::Size(width, height));
    std::vector<int64_t> input_shape = {1, 3, width, height};

    return std::make_tuple(std::move(input_image_data), std::move(input_shape));
}

std::vector<std::vector<float>> LivePortrait::MotionExtractor::process_output(
    const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};
    auto output_names = m_session->get_output_names(); // Still need names to know structure, or
                                                       // index? Original logic used index i.

    std::vector<std::vector<float>> output_data_vec;
    for (size_t i = 0; i < output_names.size();
         ++i) { // Assuming output_tensors size matches output_names size
        if (i >= output_tensors.size()) break;
        const auto* output_data = output_tensors[i].GetTensorData<float>();

        if (i <= 3) {
            output_data_vec.emplace_back(output_data, output_data + 1);
        } else if (i == 4) {
            output_data_vec.emplace_back(output_data, output_data + 3);
        } else if (i == 5 || i == 6) {
            output_data_vec.emplace_back(output_data, output_data + 21 * 3);
        }
    }
    return output_data_vec;
}

std::vector<std::vector<float>> LivePortrait::MotionExtractor::extract_motion(
    const cv::Mat& frame) const {
    if (!is_model_loaded()) { throw std::runtime_error("MotionExtractor model is not loaded"); }

    auto [input_image_data, input_shape] = prepare_input(frame);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;

    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        memory_info, input_image_data.data(), input_image_data.size(), input_shape.data(),
        input_shape.size()));

    auto output_tensor = m_session->run(input_tensors);

    return process_output(output_tensor);
}

// Generator Implementation
void LivePortrait::Generator::load_model(const std::string& path,
                                         const inference_session::Options& options) {
    m_session = InferenceSessionRegistry::get_instance().get_session(path, options);
}

bool LivePortrait::Generator::is_model_loaded() const {
    return m_session && m_session->is_model_loaded();
}

cv::Size LivePortrait::Generator::get_output_size() const {
    if (!is_model_loaded()) return {512, 512};
    auto output_node_dims = m_session->get_output_node_dims();
    int output_height = static_cast<int>(output_node_dims[0][2]);
    int output_width = static_cast<int>(output_node_dims[0][3]);
    return {output_width, output_height};
}

std::tuple<std::vector<float>, std::vector<int64_t>, std::vector<float>, std::vector<int64_t>,
           std::vector<float>, std::vector<int64_t>>
LivePortrait::Generator::prepare_input(std::vector<float>& feature_volume,
                                       std::vector<float>& source_motion_points,
                                       std::vector<float>& target_motion_points) const {
    const std::vector<int64_t> input_feature_shape{1, 32, 16, 64, 64};
    const std::vector<int64_t> input_motion_shape{1, 21, 3};
    // Note: inputs are passed by reference, but we need to return copies/moves if we follow the
    // pattern rigorously. However, the original function used them directly. To return a
    // self-contained tuple, we should copy them. Or we could return const reference wrappers if we
    // were sure of lifetime, but vector<float> return is safer.

    return std::make_tuple(feature_volume, input_feature_shape, source_motion_points,
                           input_motion_shape, target_motion_points, input_motion_shape);
}

cv::Mat LivePortrait::Generator::process_output(
    const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};

    const float* output_data = output_tensors[0].GetTensorData<float>();
    std::vector<int64_t> outs_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    const int output_height = static_cast<int>(outs_shape[2]);
    const int output_width = static_cast<int>(outs_shape[3]);

    const int channel_step = output_height * output_width;
    std::vector<cv::Mat> channel_mats(3);
    // Create matrices for each channel and scale/clamp values
    channel_mats[2] =
        cv::Mat(output_height, output_width, CV_32FC1, const_cast<float*>(output_data))
            .clone(); // R
    channel_mats[1] = cv::Mat(output_height, output_width, CV_32FC1,
                              const_cast<float*>(output_data) + channel_step)
                          .clone(); // G
    channel_mats[0] = cv::Mat(output_height, output_width, CV_32FC1,
                              const_cast<float*>(output_data) + 2 * channel_step)
                          .clone(); // B

    for (auto& mat : channel_mats) {
        mat *= 255.f;
        // Efficient clamping
        cv::threshold(mat, mat, 0, 0, cv::THRESH_TOZERO);
        cv::threshold(mat, mat, 255, 255, cv::THRESH_TRUNC);
    }

    cv::Mat result_mat;
    cv::merge(channel_mats, result_mat);
    return result_mat;
}

cv::Mat LivePortrait::Generator::generate_frame(std::vector<float>& feature_volume,
                                                std::vector<float>& source_motion_points,
                                                std::vector<float>& target_motion_points) const {
    if (!is_model_loaded()) { throw std::runtime_error("Generator model is not loaded"); }

    auto [input_feature, feature_shape, input_source, source_shape, input_target, target_shape] =
        prepare_input(feature_volume, source_motion_points, target_motion_points);

    std::vector<Ort::Value> input_tensors;
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    auto input_names = m_session->get_input_names();

    // Map inputs by name. We have 3 known inputs.
    // Order in prepare_input tuple: feature, source, target.
    // If we iterate names, we need to match them.
    // Or we can just assume names if fixed, but `get_input_names` iteration is safer if order
    // varies. But original code iterated.

    // Simplification: We have the data and shapes ready. We can just loop and create tensors.
    // BUT we need to associate data with the correct name.

    for (const auto& name : input_names) {
        if (name == "feature_volume") {
            input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, input_feature.data(), input_feature.size(), feature_shape.data(),
                feature_shape.size()));
        } else if (name == "source") {
            input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, input_source.data(), input_source.size(), source_shape.data(),
                source_shape.size()));
        } else if (name == "target") {
            input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, input_target.data(), input_target.size(), target_shape.data(),
                target_shape.size()));
        }
    }

    auto output_tensor = m_session->run(input_tensors);

    return process_output(output_tensor);
}

// LivePortrait Implementation

void LivePortrait::load_model(const std::string& feature_extractor_path,
                              const std::string& motion_extractor_path,
                              const std::string& generator_path,
                              const inference_session::Options& options) {
    m_feature_extractor.load_model(feature_extractor_path, options);
    m_motion_extractor.load_model(motion_extractor_path, options);
    m_generator.load_model(generator_path, options);
    m_generator_output_size = m_generator.get_output_size();
}

cv::Size LivePortrait::get_model_input_size() const {
    return m_feature_extractor.get_input_size();
}

cv::Mat LivePortrait::restore_expression(cv::Mat source_crop, cv::Mat target_crop,
                                         float restore_factor) {
    if (source_crop.empty() || target_crop.empty()) return {};

    if (!m_feature_extractor.is_model_loaded() || !m_motion_extractor.is_model_loaded()
        || !m_generator.is_model_loaded()) {
        throw std::runtime_error("LivePortrait models are not loaded!");
    }

    // Input Size Validation
    cv::Size required_size = get_model_input_size();
    if (source_crop.size() != required_size) {
        cv::resize(source_crop, source_crop, required_size);
    }
    if (target_crop.size() != required_size) {
        cv::resize(target_crop, target_crop, required_size);
    }

    // foundation::infrastructure::logger::ScopedTimer timer("LivePortrait::Inference");
    return apply_restore(source_crop, target_crop, restore_factor);
}

cv::Mat LivePortrait::apply_restore(const cv::Mat& cropped_source_frame,
                                    const cv::Mat& cropped_target_frame,
                                    float restore_factor) const {
    // Use ThreadPool for parallel inference
    auto feature_volume_fu = ThreadPool::instance().enqueue(
        [&] { return m_feature_extractor.extract_feature(cropped_target_frame); });
    auto source_motion_fu = ThreadPool::instance().enqueue(
        [&] { return m_motion_extractor.extract_motion(cropped_source_frame); });

    // Run target motion in current thread
    auto target_motion = m_motion_extractor.extract_motion(cropped_target_frame);

    auto feature_volume = feature_volume_fu.get();
    auto source_motion = source_motion_fu.get();

    cv::Mat rotation_mat =
        create_rotation_mat(target_motion[0][0], target_motion[1][0], target_motion[2][0]);

    std::vector<float> source_expression = source_motion[5]; // Copy
    std::vector<float> target_expression = target_motion[5]; // Ref would be bad if we modify

    // Swap expressions at specific indices
    for (auto index : {0, 4, 5, 8, 9}) { source_expression[index] = target_expression[index]; }

    cv::Mat source_expression_mat(21, 3, CV_32FC1, source_expression.data());
    cv::Mat target_expression_mat(21, 3, CV_32FC1, target_expression.data());

    source_expression_mat =
        source_expression_mat * restore_factor + target_expression_mat * (1.0f - restore_factor);
    source_expression_mat = limit_expression(source_expression_mat);

    cv::Mat target_translation_mat(21, 3, CV_32FC1);
    for (int i = 0; i < 21; ++i) {
        for (int j = 0; j < 3; ++j) {
            target_translation_mat.at<float>(i, j) = target_motion[4][j];
        }
    }
    float target_scale = target_motion[3][0];

    cv::Mat target_motion_points_mat(21, 3, CV_32FC1, target_motion[6].data());

    // Transpose rotation mat
    cv::Mat rot_t = rotation_mat.t();

    // Source motion calculation
    cv::Mat source_motion_points =
        target_scale * (target_motion_points_mat * rot_t + source_expression_mat)
        + target_translation_mat;

    // Target motion calculation (re-using target expression)
    cv::Mat target_motion_points_res =
        target_scale * (target_motion_points_mat * rot_t + target_expression_mat)
        + target_translation_mat;

    std::vector<float> source_motion_vec(source_motion_points.begin<float>(),
                                         source_motion_points.end<float>());
    std::vector<float> target_motion_vec(target_motion_points_res.begin<float>(),
                                         target_motion_points_res.end<float>());

    return m_generator.generate_frame(feature_volume, source_motion_vec, target_motion_vec);
}

std::vector<float> LivePortrait::get_input_image_data(const cv::Mat& image, const cv::Size& size) {
    cv::Mat input_image;
    cv::resize(image, input_image, size, cv::InterpolationFlags::INTER_AREA);

    if (input_image.channels() == 4) { cv::cvtColor(input_image, input_image, cv::COLOR_BGRA2BGR); }

    input_image.convertTo(input_image, CV_32FC3, 1.0 / 255.0);

    std::vector<cv::Mat> channels(3);
    cv::split(input_image, channels);

    std::vector<float> data;
    data.reserve(channels[0].total() * 3);

    // Append R
    data.insert(data.end(), channels[2].begin<float>(), channels[2].end<float>());
    // Append G
    data.insert(data.end(), channels[1].begin<float>(), channels[1].end<float>());
    // Append B
    data.insert(data.end(), channels[0].begin<float>(), channels[0].end<float>());

    return data;
}

cv::Mat LivePortrait::create_rotation_mat(float pitch, float yaw, float roll) {
    // Convert degrees to radians
    pitch = pitch * static_cast<float>(CV_PI) / 180.0f;
    yaw = yaw * static_cast<float>(CV_PI) / 180.0f;
    roll = roll * static_cast<float>(CV_PI) / 180.0f;

    // Rx
    cv::Mat Rx =
        (cv::Mat_<float>(3, 3) << 1, 0, 0, 0, cos(pitch), -sin(pitch), 0, sin(pitch), cos(pitch));

    // Ry
    cv::Mat Ry = (cv::Mat_<float>(3, 3) << cos(yaw), 0, sin(yaw), 0, 1, 0, -sin(yaw), 0, cos(yaw));

    // Rz
    cv::Mat Rz =
        (cv::Mat_<float>(3, 3) << cos(roll), -sin(roll), 0, sin(roll), cos(roll), 0, 0, 0, 1);

    // R = Rz * Ry * Rx
    return Rz * Ry * Rx;
}

cv::Mat LivePortrait::limit_expression(const cv::Mat& expression) {
    static const std::vector<float> expression_min{
        -2.88067125e-02f, -8.12731311e-02f, -1.70541159e-03f, -4.88598682e-02f, -3.32196616e-02f,
        -1.67431499e-04f, -6.75425082e-02f, -4.28681746e-02f, -1.98950816e-04f, -7.23103955e-02f,
        -3.28503326e-02f, -7.31324719e-04f, -3.87073644e-02f, -6.01546466e-02f, -5.50269964e-04f,
        -6.38048723e-02f, -2.23840728e-01f, -7.13261834e-04f, -3.02710701e-02f, -3.93195450e-02f,
        -8.24086510e-06f, -2.95799859e-02f, -5.39318882e-02f, -1.74219604e-04f, -2.92359516e-02f,
        -1.53050944e-02f, -6.30460854e-05f, -5.56493877e-03f, -2.34344602e-02f, -1.26858242e-04f,
        -4.37593013e-02f, -2.77768299e-02f, -2.70503685e-02f, -1.76926646e-02f, -1.91676542e-02f,
        -1.15090821e-04f, -8.34268332e-03f, -3.99775570e-03f, -3.27481248e-05f, -3.40162888e-02f,
        -2.81868968e-02f, -1.96679524e-04f, -2.91855410e-02f, -3.97511162e-02f, -2.81230678e-05f,
        -1.50395725e-02f, -2.49494594e-02f, -9.42573533e-05f, -1.67938769e-02f, -2.00953931e-02f,
        -4.00750607e-04f, -1.86435618e-02f, -2.48535164e-02f, -2.74416432e-02f, -4.61211195e-03f,
        -1.21660791e-02f, -2.93173041e-04f, -4.10017073e-02f, -7.43824020e-02f, -4.42762971e-02f,
        -1.90370996e-02f, -3.74363363e-02f, -1.34740388e-02f};

    static const std::vector<float> expression_max{
        4.46682945e-02f, 7.08772913e-02f, 4.08344204e-04f, 2.14308221e-02f, 6.15894832e-02f,
        4.85319615e-05f, 3.02363783e-02f, 4.45043296e-02f, 1.28298725e-05f, 3.05869691e-02f,
        3.79812494e-02f, 6.57040102e-04f, 4.45670523e-02f, 3.97259220e-02f, 7.10966764e-04f,
        9.43699256e-02f, 9.85926315e-02f, 2.02551950e-04f, 1.61131397e-02f, 2.92906128e-02f,
        3.44733417e-06f, 5.23825921e-02f, 1.07065082e-01f, 6.61510974e-04f, 2.85718683e-03f,
        8.32320191e-03f, 2.39314613e-04f, 2.57947259e-02f, 1.60935968e-02f, 2.41853559e-05f,
        4.90833223e-02f, 3.43903080e-02f, 3.22353356e-02f, 1.44766076e-02f, 3.39248963e-02f,
        1.42291479e-04f, 8.75749043e-04f, 6.82212645e-03f, 2.76097053e-05f, 1.86958015e-02f,
        3.84016186e-02f, 7.33085908e-05f, 2.01714113e-02f, 4.90544215e-02f, 2.34028921e-05f,
        2.46518422e-02f, 3.29151377e-02f, 3.48571630e-05f, 2.22457591e-02f, 1.21796541e-02f,
        1.56396593e-04f, 1.72109623e-02f, 3.01626958e-02f, 1.36556877e-02f, 1.83460284e-02f,
        1.61141958e-02f, 2.87440169e-04f, 3.57594155e-02f, 1.80554688e-01f, 2.75554154e-02f,
        2.17450950e-02f, 8.66811201e-02f, 3.34241726e-02f};

    static cv::Mat expression_min_mat(21, 3, CV_32FC1, const_cast<float*>(expression_min.data()));
    static cv::Mat expression_max_mat(21, 3, CV_32FC1, const_cast<float*>(expression_max.data()));

    cv::Mat limited_expression;
    cv::max(expression, expression_min_mat, limited_expression);
    cv::min(limited_expression, expression_max_mat, limited_expression);

    return limited_expression;
}

} // namespace domain::face::expression