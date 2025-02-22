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
    std::vector<Face::Landmarks> target_faces_5_landmarks;
    std::shared_ptr<cv::Mat> target_frame = nullptr;
    ushort faceBlend{80};
    FaceMaskerHub::ArgsForGetBestMask args_for_get_best_mask{
        .faceMaskersTypes = {FaceMaskerHub::Type::Box},
        .boxMaskBlur = {0.5},
        .boxMaskPadding = std::array{0, 0, 0, 0}};
};

class CodeFormer final : public FaceEnhancerBase, public ffc::InferenceSession {
public:
    explicit CodeFormer(const std::shared_ptr<Ort::Env>& env);
    ~CodeFormer() override = default;

    [[nodiscard]] std::string getProcessorName() const override;

    void loadModel(const std::string& modelPath, const Options& options) override;

    cv::Mat enhanceFace(const CodeFormerInput& input) const;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};
    cv::Size m_size{0, 0};
    face_helper::WarpTemplateType m_warpTemplateType = face_helper::WarpTemplateType::Ffhq_512;

    static std::vector<float> getInputImageData(const cv::Mat& croppedImage);
    cv::Mat applyEnhance(const cv::Mat& croppedFrame) const;
};
} // namespace ffc::faceEnhancer
