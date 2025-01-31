/**
 ******************************************************************************
 * @file           : face_classifier_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_classifier_hub:face_classifier_base;
export import face;
export import inference_session;

namespace ffc::faceClassifier {

export class FaceClassifierBase : public InferenceSession {
public:
    explicit FaceClassifierBase(const std::shared_ptr<Ort::Env> &env = nullptr);
    virtual ~FaceClassifierBase() = default;

    struct Result {
        Face::Race race;
        Face::Gender gender;
        Face::Age age;
    };

    virtual Result classify(const cv::Mat &image, const Face::Landmark &faceLandmark5) = 0;
};

} // namespace ffc::faceClassifier
