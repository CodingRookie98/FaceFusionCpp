/**
 ******************************************************************************
 * @file           : face_enhancer_gfpgan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_enhancer:gfp_gan;
import face;
import :face_enhancer_base;
import face_masker_hub;
import face_helper;

export namespace ffc::faceEnhancer {
using namespace faceMasker;

struct GFP_GAN_Input {
    cv::Mat* targetFrame = nullptr;
    std::vector<Face>* targetFaces = nullptr;
    ushort faceBlend{80};
    FaceMaskerHub::Args4GetBestMask args4_get_best_mask{
        .faceMaskersTypes = {FaceMaskerHub::Type::Box},
        .boxMaskBlur = {0.5},
        .boxMaskPadding = std::array{0, 0, 0, 0}};
};

class GFP_GAN final : public FaceEnhancerBase, public ffc::InferenceSession {
public:
    explicit GFP_GAN(const std::shared_ptr<Ort::Env>& env);
    ~GFP_GAN() override = default;

    [[nodiscard]] std::string getProcessorName() const override;

    void loadModel(const std::string& modelPath, const Options& options) override;

    [[nodiscard]] cv::Mat enhanceFace(const GFP_GAN_Input& input) const;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};
    cv::Size m_size{0, 0};
    FaceHelper::WarpTemplateType m_warpTemplateType{FaceHelper::WarpTemplateType::Ffhq_512};

    [[nodiscard]] static std::vector<float> getInputImageData(const cv::Mat& croppedImage);
    [[nodiscard]] cv::Mat applyEnhance(const cv::Mat& croppedFrame) const;
};
} // namespace ffc::faceEnhancer
