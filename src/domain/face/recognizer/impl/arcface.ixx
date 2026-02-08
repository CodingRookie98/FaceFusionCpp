module;
#include <vector>
#include <string>
#include <utility>
#include <tuple>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.recognizer:impl.arcface;

import :api;
import domain.face;
import foundation.ai.inference_session;

namespace domain::face::recognizer {

export class ArcFace : public FaceRecognizer {
public:
    explicit ArcFace();
    ~ArcFace() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;

    std::pair<types::Embedding, types::Embedding> recognize(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) override;

private:
    int m_input_width = 112;
    int m_input_height = 112;

    [[nodiscard]] std::tuple<std::vector<float>, std::vector<int64_t>> prepare_input(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) const;
    [[nodiscard]] std::pair<types::Embedding, types::Embedding> process_output(
        const std::vector<Ort::Value>& output_tensors) const;
};

} // namespace domain::face::recognizer
