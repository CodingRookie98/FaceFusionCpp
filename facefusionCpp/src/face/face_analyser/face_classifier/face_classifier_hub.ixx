/**
 ******************************************************************************
 * @file           : face_classifiers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <shared_mutex>

export module face_classifier_hub;
import :face_classifier_base;

export namespace ffc::faceClassifier {

class FaceClassifierHub {
public:
    explicit FaceClassifierHub(const std::shared_ptr<Ort::Env> &env = nullptr, const ffc::InferenceSession::Options &options = ffc::InferenceSession::Options());
    ~FaceClassifierHub();

    enum class Type {
        FairFace,
    };
    FaceClassifierBase::Result
    classify(const cv::Mat &image, const Face::Landmark &faceLandmark5, const Type &type = Type::FairFace);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::shared_mutex m_sharedMutex;
    std::unordered_map<Type, FaceClassifierBase *> m_classifiers;
    ffc::InferenceSession::Options m_ISOptions;

    FaceClassifierBase *getFaceClassifier(Type type);
};

} // namespace ffc::faceClassifier