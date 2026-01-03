/**
 ******************************************************************************
 * @file           : face_landmarker_peppawutz.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-26
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_landmarker_hub:peppawutz;
import :face_landmarker_base;
export import face;

namespace ffc::face_landmarker {
export class Peppawutz final : public FaceLandmarkerBase {
public:
    explicit Peppawutz(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~Peppawutz() override = default;

    std::tuple<Face::Landmarks, float> detect(const cv::Mat& visionFrame, const BBox& bBox) const;
    void load_model(const std::string& modelPath, const Options& options) override;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};
    cv::Size m_inputSize{256, 256};
    [[nodiscard]] std::tuple<std::vector<float>, cv::Mat> preProcess(const cv::Mat& visionFrame, const BBox& bBox) const;
};
} // namespace ffc::face_landmarker