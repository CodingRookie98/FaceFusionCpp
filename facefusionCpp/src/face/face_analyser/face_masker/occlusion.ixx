/**
 ******************************************************************************
 * @file           : face_masker_occlusion.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_masker_hub:occlusion;
import :face_masker_base;

export namespace ffc::faceMasker {
class Occlusion final : public FaceMaskerBase {
public:
    explicit Occlusion(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~Occlusion() override = default;

    cv::Mat createOcclusionMask(const cv::Mat &cropVisionFrame) const;

    void LoadModel(const std::string &modelPath, const Options &options) override;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};

    std::vector<float> getInputImageData(const cv::Mat &cropVisionFrame) const;
};

} // namespace ffc::faceMasker