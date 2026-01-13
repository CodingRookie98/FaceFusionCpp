module;
#include <memory>
#include <array>
#include <vector>
#include <opencv2/core/types.hpp>

export module domain.face.classifier:fair_face;
import :api;
import domain.face.helper;
import foundation.ai.inference_session;

export namespace domain::face::classifier {
    class FairFace final : public foundation::ai::inference_session::InferenceSession, public IFaceClassifier {
    public:
        FairFace();
        ~FairFace() override = default;

        void load_model(const std::string& model_path,
                        const foundation::ai::inference_session::Options& options = {}) override;

        ClassificationResult classify(const cv::Mat& image, const domain::face::types::Landmarks& face_landmark_5) override;

    private:
        domain::face::helper::WarpTemplateType m_WarpTemplateType =
            domain::face::helper::WarpTemplateType::Arcface_112_v2;
        cv::Size m_size{224, 224};
        std::array<float, 3> m_mean{0.485f, 0.456f, 0.406f};
        std::array<float, 3> m_standardDeviation{0.229f, 0.224f, 0.225f};
        int m_inputWidth{0};
        int m_inputHeight{0};

        std::vector<float> getInputImageData(const cv::Mat& image,
                                             const domain::face::types::Landmarks& face_landmark_5) const;
        static domain::face::Gender categorizeGender(int64_t genderId);
        static domain::face::AgeRange categorizeAge(int64_t age_id);
        static domain::face::Race categorizeRace(int64_t raceId);
    };
}
