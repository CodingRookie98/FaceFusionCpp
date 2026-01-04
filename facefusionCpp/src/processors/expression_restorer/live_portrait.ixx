/**
 ******************************************************************************
 * @file           : expression_restorer.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-23
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module expression_restorer:live_portrait;
import :expression_restorer_basee;
import face_helper;

export namespace ffc::expressionRestore {
using namespace face_masker;

// You need to ensure that the source_faces_5_landmarks.size == target_faces_5_landmarks.size
// otherwise the returned value is target_frame.clone()
struct LivePortraitInput {
    std::shared_ptr<cv::Mat> source_frame{nullptr};
    std::vector<Face::Landmarks> source_faces_5_landmarks;
    std::shared_ptr<cv::Mat> target_frame{nullptr};
    std::vector<Face::Landmarks> target_faces_5_landmarks;
    float restoreFactor{0.96};
    std::unordered_set<FaceMaskerHub::Type> faceMaskersTypes{FaceMaskerHub::Type::Box};
    float boxMaskBlur{0.5};
    std::array<int, 4> boxMaskPadding{0, 0, 0, 0};
};

class LivePortrait final : public ExpressionRestorerBase {
public:
    explicit LivePortrait(const std::shared_ptr<Ort::Env>& env);
    ~LivePortrait() override = default;

    [[nodiscard]] std::string get_processor_name() const override;

    void loadModel(const std::string& featureExtractorPath,
                   const std::string& motionExtractorPath,
                   const std::string& generatorPath,
                   const ai::InferenceSession::Options& options);

    [[nodiscard]] bool isModelLoaded() const {
        return m_featureExtractor.is_model_loaded()
            && m_motionExtractor.is_model_loaded()
            && m_generator.is_model_loaded();
    }

    [[nodiscard]] cv::Mat restoreExpression(const LivePortraitInput& input);

private:
    class FeatureExtractor final : public ai::InferenceSession {
    public:
        explicit FeatureExtractor(const std::shared_ptr<Ort::Env>& env);
        [[nodiscard]] std::vector<float> extractFeature(const cv::Mat& frame) const;
    };

    class MotionExtractor final : public ai::InferenceSession {
    public:
        explicit MotionExtractor(const std::shared_ptr<Ort::Env>& env);
        [[nodiscard]] std::vector<std::vector<float>> extractMotion(const cv::Mat& frame) const;
    };

    class Generator final : public ai::InferenceSession {
    public:
        explicit Generator(const std::shared_ptr<Ort::Env>& env);
        [[nodiscard]] cv::Mat generateFrame(std::vector<float>& featureVolume,
                                            std::vector<float>& sourceMotionPoints,
                                            std::vector<float>& targetMotionPoints) const;
        [[nodiscard]] cv::Size getOutputSize() const {
            int outputHeight = m_output_node_dims[0][2];
            int outputWidth = m_output_node_dims[0][3];
            return {outputWidth, outputHeight};
        }
    };

    cv::Size m_generatorOutputSize{512, 512};
    face_helper::WarpTemplateType m_warpTemplateType = face_helper::WarpTemplateType::Arcface_128_v2;
    float m_restoreFactor = 0.96;
    std::shared_ptr<Ort::Env> m_env;
    FeatureExtractor m_featureExtractor;
    MotionExtractor m_motionExtractor;
    Generator m_generator;

    static std::vector<float> getInputImageData(const cv::Mat& image, const cv::Size& size);

    [[nodiscard]] cv::Mat applyRestore(const cv::Mat& croppedSourceFrame, const cv::Mat& croppedTargetFrame) const;

    [[nodiscard]] static cv::Mat createRotationMat(float pitch, float yaw, float roll);

    [[nodiscard]] static cv::Mat limitExpression(const cv::Mat& expression);
};
} // namespace ffc::expressionRestore
