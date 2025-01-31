/**
 ******************************************************************************
 * @file           : fr_arc_w_600_k_r_50.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <vector>
#include <memory>

export module face_recognizer_hub:arc_w600k_r50;
import :face_recognizer_base;

namespace ffc::faceRecognizer {
export class ArcW600kR50 : public FaceRecognizerBase {
public:
    // model: arcface_w600k_r50.onnx
    explicit ArcW600kR50(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~ArcW600kR50() override = default;

    // return: [0] embedding, [1] normedEmbedding
    std::array<std::vector<float>, 2> recognize(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5) override;
    void loadModel(const std::string &modelPath, const Options &options) override;

private:
    std::vector<float> preProcess(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5_68) const;
    int m_inputWidth{0};
    int m_inputHeight{0};
};
} // namespace ffc::faceRecognizer
