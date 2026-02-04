module;
#include <memory>
#include <array>
#include <vector>
#include <string>
#include <opencv2/core/types.hpp>
#include <onnxruntime_cxx_api.h>
#include <utility>

export module domain.face.classifier:fair_face;
import :api;
import domain.face.helper;
import foundation.ai.inference_session;

export namespace domain::face::classifier {
class FairFace final : public IFaceClassifier {
public:
    FairFace();
    ~FairFace() override = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override;

    ClassificationResult classify(const cv::Mat& image,
                                  const domain::face::types::Landmarks& face_landmark_5) override;

    [[nodiscard]] bool is_model_loaded() const { return m_session && m_session->is_model_loaded(); }

    [[nodiscard]] std::vector<std::vector<int64_t>> get_input_node_dims() const {
        if (!m_session) return {};
        return m_session->get_input_node_dims();
    }

    [[nodiscard]] std::vector<std::vector<int64_t>> get_output_node_dims() const {
        if (!m_session) return {};
        return m_session->get_output_node_dims();
    }

    std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors) {
        if (!m_session) return {};
        return m_session->run(input_tensors);
    }

private:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
    domain::face::helper::WarpTemplateType m_warp_template_type =
        domain::face::helper::WarpTemplateType::Arcface112V2;
    cv::Size m_size{224, 224};
    std::array<float, 3> m_mean{0.485f, 0.456f, 0.406f};
    std::array<float, 3> m_standard_deviation{0.229f, 0.224f, 0.225f};
    int m_input_width{0};
    int m_input_height{0};

    std::pair<std::vector<float>, std::vector<int64_t>> prepare_input(
        const cv::Mat& image, const domain::face::types::Landmarks& face_landmark_5) const;
    ClassificationResult process_output(const std::vector<Ort::Value>& outputTensor) const;
    static domain::face::Gender categorizeGender(int64_t genderId);
    static domain::face::AgeRange categorizeAge(int64_t age_id);
    static domain::face::Race categorizeRace(int64_t raceId);
};
} // namespace domain::face::classifier
