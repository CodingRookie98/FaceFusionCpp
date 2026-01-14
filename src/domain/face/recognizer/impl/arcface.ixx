module;
#include <vector>
#include <string>
#include <utility>
#include <opencv2/opencv.hpp>

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
                    const foundation::ai::inference_session::Options& options);

    std::pair<types::Embedding, types::Embedding> recognize(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) override;

private:
    int m_input_width = 112;
    int m_input_height = 112;

    std::vector<float> pre_process(const cv::Mat& vision_frame,
                                   const types::Landmarks& face_landmark_5) const;
};

} // namespace domain::face::recognizer
