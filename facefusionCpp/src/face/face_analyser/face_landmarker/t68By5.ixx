/**
 ******************************************************************************
 * @file           : face_landmarker_68_5.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_landmarker_hub:t68by5;
import :face_landmarker_base;
import face;

namespace ffc::faceLandmarker {
export class T68By5 final : public FaceLandmarkerBase {
public:
    explicit T68By5(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~T68By5() override = default;

    [[nodiscard]] Face::Landmarks detect(const Face::Landmarks &faceLandmark5) const;
    void loadModel(const std::string &modelPath, const Options &options) override;

private:
    static std::tuple<std::vector<float>, cv::Mat> preProcess(const Face::Landmarks &faceLandmark5);
    int m_inputHeight{0};
    int m_inputWidth{0};
};

} // namespace ffc::faceLandmarker