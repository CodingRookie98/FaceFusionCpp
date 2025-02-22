/**
 ******************************************************************************
 * @file           : face_recognizer_hub.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_recognizer_hub;
import :face_recognizer_base;

export namespace ffc::faceRecognizer {
class FaceRecognizerHub {
public:
    explicit FaceRecognizerHub(const std::shared_ptr<Ort::Env> &env = nullptr,
                               const ffc::InferenceSession::Options &ISOptions = {});
    ~FaceRecognizerHub();

    enum class Type {
        Arc_w600k_r50,
    };

    // return: [0] embedding, [1] normedEmbedding
    std::array<Face::Embeddings, 2>
    recognize(const cv::Mat &visionFrame, const Face::Landmarks &faceLandmark5, const Type &type);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::shared_mutex m_sharedMutex;
    std::unordered_map<Type, FaceRecognizerBase *> m_recognizers;
    InferenceSession::Options m_ISOptions;

    [[nodiscard]] FaceRecognizerBase *getRecognizer(const Type &type);
};
} // namespace ffc::faceRecognizer