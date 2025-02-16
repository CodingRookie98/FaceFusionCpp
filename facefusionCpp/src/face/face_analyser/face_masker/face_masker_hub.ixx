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

export namespace ffc::faceMasker {
class FaceMaskerHub {
public:
    explicit FaceMaskerHub(const std::shared_ptr<Ort::Env>& env = nullptr,
                           const InferenceSession::Options& options = InferenceSession::Options{});
    ~FaceMaskerHub();

    enum class Type {
        Box,
        Occlusion,
        Region
    };

    struct Args4GetBestMask {
        std::unordered_set<Type> faceMaskersTypes{Type::Box};
        std::optional<ModelManager::Model> occluder_model{std::nullopt};
        std::optional<ModelManager::Model> parser_model{std::nullopt};
        std::optional<float> boxMaskBlur{std::nullopt};
        std::optional<std::array<int, 4>> boxMaskPadding{std::nullopt};
        std::optional<cv::Size> boxSize{std::nullopt};
        std::optional<const cv::Mat*> occlusionFrame{std::nullopt};
        std::optional<const cv::Mat*> regionFrame{std::nullopt};
        std::optional<std::unordered_set<FaceMaskerRegion::Region>> faceMaskerRegions{std::nullopt};
    };

    static cv::Mat createStaticBoxMask(const cv::Size& cropSize, const float& faceMaskBlur = 0.3f,
                                       const std::array<int, 4>& faceMaskPadding = {0, 0, 0, 0});

    cv::Mat createRegionMask(const cv::Mat& inputImage,
                             const ModelManager::Model& parser_model,
                             const std::unordered_set<FaceMaskerRegion::Region>& regions);

    cv::Mat createOcclusionMask(const cv::Mat& cropVisionFrame,
                                const ModelManager::Model& occluder_model);

    static cv::Mat getBestMask(const std::vector<cv::Mat>& masks);

    [[nodiscard]] cv::Mat getBestMask(const Args4GetBestMask& func_gbm_args);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<Type, FaceMaskerBase*> m_maskers;
    std::shared_mutex m_sharedMutex;
    InferenceSession::Options m_options;

    FaceMaskerBase* getMasker(const Type& type, const ModelManager::Model& model);
};

} // namespace ffc::faceMasker