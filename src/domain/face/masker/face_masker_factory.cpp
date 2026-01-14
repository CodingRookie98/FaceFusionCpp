module;
#include <memory>
#include <string>

module domain.face.masker;
import :factory;
import :impl_occlusion;
import :impl_region;
import foundation.ai.inference_session;

namespace domain::face::masker {

std::unique_ptr<IFaceOccluder> create_occlusion_masker(
    const std::string& model_path, const foundation::ai::inference_session::Options& options) {
    auto masker = std::make_unique<OcclusionMasker>();
    masker->load_model(model_path, options);
    return masker;
}

std::unique_ptr<IFaceRegionMasker> create_region_masker(
    const std::string& model_path, const foundation::ai::inference_session::Options& options) {
    auto masker = std::make_unique<RegionMasker>();
    masker->load_model(model_path, options);
    return masker;
}

} // namespace domain::face::masker
