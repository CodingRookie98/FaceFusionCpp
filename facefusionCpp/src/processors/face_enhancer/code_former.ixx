/**
 ******************************************************************************
 * @file           : code_former.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-22
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_enhancer:code_former;
import face;
import face_masker_hub;
import :face_enhancer_base;
import face_helper;

export namespace ffc::faceEnhancer {
using namespace faceMasker;

struct CodeFormerInput {
    cv::Mat *targetFrame = nullptr;
    std::vector<Face> *targetFaces = nullptr;
    ushort faceBlend{80};
    std::unordered_set<FaceMaskerHub::Type> faceMaskersTypes{FaceMaskerHub::Type::Box};
    float boxMaskBlur{0.5};
    std::array<int, 4> boxMaskPadding{0, 0, 0, 0};
};

class CodeFormer final : public FaceEnhancerBase, public ffc::InferenceSession {
public:
    CodeFormer(const std::shared_ptr<Ort::Env> &env);
    ~CodeFormer() override = default;

    std::string getProcessorName() const override;

    void loadModel(const std::string &modelPath, const Options &options) override;

    cv::Mat enhanceFace(const CodeFormerInput &_input) const;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};
    cv::Size m_size{0, 0};
    FaceHelper::WarpTemplateType m_warpTemplateType = FaceHelper::WarpTemplateType::Ffhq_512;

    static std::vector<float> getInputImageData(const cv::Mat &croppedImage);
    cv::Mat applyEnhance(const cv::Mat &croppedFrame) const;
};
} // namespace faceEnhancer
