module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <tuple>

module domain.face.recognizer;

import :impl.arcface;
import domain.face.helper;

namespace domain::face::recognizer {

ArcFace::ArcFace() : FaceRecognizer() {}

void ArcFace::load_model(const std::string& model_path,
                         const foundation::ai::inference_session::Options& options) {
    FaceRecognizer::load_model(model_path, options);
    auto input_dims = get_input_node_dims();
    if (!input_dims.empty() && input_dims[0].size() >= 4) {
        m_input_width = static_cast<int>(input_dims[0][2]);
        m_input_height = static_cast<int>(input_dims[0][3]);
    }
}

std::tuple<std::vector<float>, std::vector<int64_t>> ArcFace::prepare_input(
    const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) const {
    using namespace domain::face::helper;

    // Get warp template
    auto warp_template = get_warp_template(WarpTemplateType::Arcface112V2);

    // Warp face
    cv::Mat cropped_frame;
    std::tie(cropped_frame, std::ignore) = warp_face_by_face_landmarks_5(
        vision_frame, face_landmark_5, warp_template, cv::Size(112, 112));

    // Convert to float and normalize
    // Legacy: (pixel * (1/127.5)) - 1.0
    cv::Mat float_frame;
    cropped_frame.convertTo(float_frame, CV_32FC3, 1.0 / 127.5, -1.0);

    // HWC to CHW (Split channels)
    std::vector<cv::Mat> channels(3);
    cv::split(float_frame, channels);

    // Create input data buffer (RGB planar)
    // Legacy legacy used: BGR channels split, then stored as RGB order in inputData
    // memcpy(..., bgr[2].data, ...) -> R
    // memcpy(..., bgr[1].data, ...) -> G
    // memcpy(..., bgr[0].data, ...) -> B
    // OpenCV loads as BGR by default. split gives B, G, R.
    // So legacy code puts channels[2] (R), then channels[1] (G), then channels[0] (B).

    std::vector<float> input_data;
    int image_area = m_input_height * m_input_width;
    input_data.resize(3 * image_area);

    // R channel
    std::memcpy(input_data.data(), channels[2].data, image_area * sizeof(float));
    // G channel
    std::memcpy(input_data.data() + image_area, channels[1].data, image_area * sizeof(float));
    // B channel
    std::memcpy(input_data.data() + image_area * 2, channels[0].data, image_area * sizeof(float));

    std::vector<int64_t> input_shape = {1, 3, m_input_height, m_input_width};

    return std::make_tuple(std::move(input_data), std::move(input_shape));
}

std::pair<types::Embedding, types::Embedding> ArcFace::process_output(
    const std::vector<Ort::Value>& output_tensors) const {
    if (output_tensors.empty()) return {};

    // Process output
    const float* raw_data = output_tensors[0].GetTensorData<float>();
    auto shape_info = output_tensors[0].GetTensorTypeAndShapeInfo();
    size_t feature_len = shape_info.GetShape()[1]; // Should be 512

    types::Embedding embedding(feature_len);
    std::memcpy(embedding.data(), raw_data, feature_len * sizeof(float));

    // Normalize
    types::Embedding normed_embedding(feature_len);
    double norm = cv::norm(embedding, cv::NORM_L2);

    // Avoid division by zero
    float norm_val = (norm > 1e-6) ? static_cast<float>(norm) : 1.0f;

    for (size_t i = 0; i < feature_len; ++i) { normed_embedding[i] = embedding[i] / norm_val; }

    return {embedding, normed_embedding};
}

std::pair<types::Embedding, types::Embedding> ArcFace::recognize(
    const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) {
    auto [input_data, input_shape] = prepare_input(vision_frame, face_landmark_5);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> input_tensors;

    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size()));

    auto output_tensors = run(input_tensors);

    return process_output(output_tensors);
}

} // namespace domain::face::recognizer
