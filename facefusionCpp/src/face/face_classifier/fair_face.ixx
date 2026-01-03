/**
 ******************************************************************************
 * @file           : face_classifier_fair_face.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_classifier_hub:fair_face;
import :face_classifier_base;
import face_helper;

namespace ffc::face_classifier {
export class FairFace final : public FaceClassifierBase {
public:
    explicit FairFace(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~FairFace() override = default;

    Result classify(const cv::Mat& image, const Face::Landmarks& faceLandmark5) override;

    void load_model(const std::string& modelPath, const Options& options) override;

private:
    face_helper::WarpTemplateType m_WarpTemplateType = face_helper::WarpTemplateType::Arcface_112_v2;
    cv::Size m_size{224, 224};
    std::array<float, 3> m_mean{0.485, 0.456, 0.406};
    std::array<float, 3> m_standardDeviation{0.229, 0.224, 0.225};
    int m_inputWidth{0};
    int m_inputHeight{0};

    std::vector<float> getInputImageData(const cv::Mat& image, const Face::Landmarks& faceLandmark5) const;
    static Gender categorizeGender(const int64_t& genderId);
    static AgeRange categorizeAge(const int64_t& age_id);
    static Race categorizeRace(const int64_t& raceId);
};
} // namespace ffc::face_classifier