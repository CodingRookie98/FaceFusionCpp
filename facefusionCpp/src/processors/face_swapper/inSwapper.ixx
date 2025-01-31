/**
 ******************************************************************************
 * @file           : face_swapper_inswaper_128.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_swapper:inSwapper;
import :face_swapper_base;
export import face;
import face_helper;
import face_masker_hub;
import inference_session;

export namespace ffc::faceSwapper {

using namespace faceMasker;

// 指针类型的数据请自行进行内存管理
struct InSwapperInput {
    Face *sourceFace = nullptr;
    std::vector<Face> *targetFaces = nullptr;
    cv::Mat *targetFrame = nullptr;
    std::unordered_set<FaceMaskerHub::Type> faceMaskersTypes{FaceMaskerHub::Type::Box};
    std::optional<std::unordered_set<FaceMaskerRegion::Region>> faceMaskerRegions = std::nullopt;
    float boxMaskBlur{0.5};
    std::array<int, 4> boxMaskPadding{0, 0, 0, 0};
};

class InSwapper final : public FaceSwapperBase, public InferenceSession {
public:
    explicit InSwapper(const std::shared_ptr<Ort::Env> &env);
    ~InSwapper() override = default;

    // must loadModel before swapFace
    cv::Mat swapFace(const InSwapperInput &input);

    void loadModel(const std::string &modelPath, const Options &options) override;

    [[nodiscard]] std::string getProcessorName() const override;

private:
    cv::Size m_size{0, 0};
    const std::vector<float> m_mean{0.0, 0.0, 0.0};
    const std::vector<float> m_standardDeviation{1.0, 1.0, 1.0};
    const FaceHelper::WarpTemplateType m_warpTemplateType{FaceHelper::WarpTemplateType::Arcface_128_v2};
    int m_inputHeight{0};
    int m_inputWidth{0};
    std::vector<float> m_initializerArray;

    [[nodiscard]] cv::Mat applySwap(const Face::Embedding &sourceEmbedding, const cv::Mat &croppedTargetFrame) const;
    [[nodiscard]] std::vector<float> prepareSourceEmbedding(const Face::Embedding &sourceEmbedding) const;
    [[nodiscard]] std::vector<float> getInputImageData(const cv::Mat &croppedTargetFrame) const;
    void init();
};
} // namespace ffc::faceSwapper