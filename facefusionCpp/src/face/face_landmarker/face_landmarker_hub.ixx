/**
 ******************************************************************************
 * @file           : face_landmarkers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_landmarker_hub;
import :face_landmarker_base;
import face;

export namespace ffc::face_landmarker {
class FaceLandmarkerHub {
public:
    explicit FaceLandmarkerHub(const std::shared_ptr<Ort::Env>& env = nullptr,
                               const InferenceSession::Options& ISOptions = InferenceSession::Options{});
    ~FaceLandmarkerHub();

    enum class Type {
        _2DFAN,
        PEPPA_WUTZ,
    };
    struct Options {
        std::unordered_set<Type> types{Type::_2DFAN};
        double angle{0.0};
        float minScore{0.5f};
    };

    Face::Landmarks expand_landmark68_from_5(const Face::Landmarks& landmark5);
    std::tuple<Face::Landmarks, float> detect_landmark68(const cv::Mat& visionFrame, const cv::Rect2f& bbox,
                                                         const Options& options);

private:
    enum class LandmarkerModel {
        _2DFAN,
        _68By5,
        PEPPA_WUTZ,
    };
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<LandmarkerModel, FaceLandmarkerBase*> m_landmarkers;
    std::shared_mutex m_shared_mutex_landmarkers;
    ai::InferenceSession::Options m_is_options; //< 推理会话选项

    FaceLandmarkerBase* get_landmarker(const LandmarkerModel& type);
};

} // namespace ffc::face_landmarker