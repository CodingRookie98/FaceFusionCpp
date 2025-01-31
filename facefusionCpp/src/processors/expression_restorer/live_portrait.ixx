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
using namespace faceMasker;

struct LivePortraitInput {
    cv::Mat *sourceFrame = nullptr;
    cv::Mat *targetFrame = nullptr;
    std::vector<Face> *targetFaces = nullptr;
    float restoreFactor{0.96};
    std::unordered_set<FaceMaskerHub::Type> faceMaskersTypes{FaceMaskerHub::Type::Box};
    float boxMaskBlur{0.5};
    std::array<int, 4> boxMaskPadding{0, 0, 0, 0};
};

class LivePortrait final : public ExpressionRestorerBase {
public:
    explicit LivePortrait(const std::shared_ptr<Ort::Env> &env);
    ~LivePortrait() override = default;

    [[nodiscard]] std::string getProcessorName() const override;

    void loadModel(const std::string &featureExtractorPath,
                   const std::string &motionExtractorPath,
                   const std::string &generatorPath,
                   const InferenceSession::Options &options);

    [[nodiscard]] bool isModelLoaded() const {
        return m_featureExtractor.isModelLoaded()
               && m_motionExtractor.isModelLoaded()
               && m_generator.isModelLoaded();
    }

    [[nodiscard]] cv::Mat restoreExpression(const LivePortraitInput &input);

private:
    class FeatureExtractor final : public ffc::InferenceSession {
    public:
        explicit FeatureExtractor(const std::shared_ptr<Ort::Env> &env);
        [[nodiscard]] std::vector<float> extractFeature(const cv::Mat &frame) const;
    };

    class MotionExtractor final : public ffc::InferenceSession {
    public:
        explicit MotionExtractor(const std::shared_ptr<Ort::Env> &env);
        [[nodiscard]] std::vector<std::vector<float>> extractMotion(const cv::Mat &frame) const;
    };

    class Generator final : public InferenceSession {
    public:
        explicit Generator(const std::shared_ptr<Ort::Env> &env);
        [[nodiscard]] cv::Mat generateFrame(std::vector<float> &featureVolume,
                                            std::vector<float> &sourceMotionPoints,
                                            std::vector<float> &targetMotionPoints) const;
        [[nodiscard]] cv::Size getOutputSize() const {
            int outputHeight = m_outputNodeDims[0][2];
            int outputWidth = m_outputNodeDims[0][3];
            return {outputWidth, outputHeight};
        }
    };

    cv::Size m_generatorOutputSize{512, 512};
    FaceHelper::WarpTemplateType m_warpTemplateType = FaceHelper::WarpTemplateType::Arcface_128_v2;
    float m_restoreFactor = 0.96;
    std::shared_ptr<Ort::Env> m_env;
    FeatureExtractor m_featureExtractor;
    MotionExtractor m_motionExtractor;
    Generator m_generator;

    static std::vector<float> getInputImageData(const cv::Mat &image, const cv::Size &size);

    [[nodiscard]] cv::Mat applyRestore(const cv::Mat &croppedSourceFrame, const cv::Mat &croppedTargetFrame) const;

    [[nodiscard]] static cv::Mat createRotationMat(float pitch, float yaw, float roll);

    [[nodiscard]] static cv::Mat limitExpression(const cv::Mat &expression);
};
} // namespace ffc::expressionRestore
