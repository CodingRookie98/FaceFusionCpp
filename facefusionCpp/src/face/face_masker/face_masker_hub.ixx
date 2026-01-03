/**
 ******************************************************************************
 * @file           : face_maskers_.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <shared_mutex>
#include <unordered_set>
#include <opencv2/opencv.hpp>

export module face_masker_hub;
import :face_masker_base;
export import :region;
import :occlusion;
import inference_session;
import model_manager;

export namespace ffc::face_masker {

using namespace ai::model_manager;
using namespace infra;

class FaceMaskerHub {
public:
    explicit FaceMaskerHub(const std::shared_ptr<Ort::Env>& env = nullptr,
                           const ai::InferenceSession::Options& options = ai::InferenceSession::Options{});
    ~FaceMaskerHub();

    enum class Type {
        Box,
        Occlusion,
        Region
    };

    struct ArgsForGetBestMask {
        std::unordered_set<Type> faceMaskersTypes{Type::Box};
        std::optional<Model> occluder_model{std::nullopt};
        std::optional<Model> parser_model{std::nullopt};
        std::optional<float> boxMaskBlur{std::nullopt};
        std::optional<std::array<int, 4>> boxMaskPadding{std::nullopt};
        std::optional<cv::Size> boxSize{std::nullopt};
        std::optional<const cv::Mat*> occlusionFrame{std::nullopt};
        std::optional<const cv::Mat*> regionFrame{std::nullopt};
        std::optional<std::unordered_set<FaceMaskerRegion::Region>> faceMaskerRegions{std::nullopt};
    };

    static cv::Mat create_static_box_mask(const cv::Size& cropSize, const float& faceMaskBlur = 0.3f,
                                          const std::array<int, 4>& faceMaskPadding = {0, 0, 0, 0});

    cv::Mat create_region_mask(const cv::Mat& inputImage,
                               const Model& parser_model,
                               const std::unordered_set<FaceMaskerRegion::Region>& regions);

    cv::Mat create_occlusion_mask(const cv::Mat& cropVisionFrame,
                                  const Model& occluder_model);

    static cv::Mat get_best_mask(const std::vector<cv::Mat>& masks);

    [[nodiscard]] cv::Mat get_best_mask(const ArgsForGetBestMask& func_gbm_args);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<Type, FaceMaskerBase*> m_maskers;
    std::shared_mutex m_sharedMutex;
    ai::InferenceSession::Options m_options;

    FaceMaskerBase* get_masker(const Type& type, const Model& model);
};

} // namespace ffc::face_masker