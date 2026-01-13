/**
 ******************************************************************************
 * @file           : face_landmarker_2_dfan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_landmarker_hub:t2dfan;
import face;
import :face_landmarker_base;

namespace ffc::face_landmarker {
export class T2dfan final : public FaceLandmarkerBase {
public:
    explicit T2dfan(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~T2dfan() override = default;

    // Return the coordinates and confidence values for the 68 facial landmarks
    // 1st is landmark, 2nd is confidence
    [[nodiscard]] std::tuple<Face::Landmarks, float> detect(const cv::Mat& visionFrame,
                                                            const cv::Rect2f& bBox) const;

    void load_model(const std::string& modelPath, const Options& options) override;

private:
    int m_inputHeight{0};
    int m_inputWidth{0};
    cv::Size m_inputSize{256, 256};
    [[nodiscard]] std::tuple<std::vector<float>, cv::Mat> preProcess(const cv::Mat& visionFrame,
                                                                     const cv::Rect2f& bBox) const;
};
} // namespace ffc::face_landmarker