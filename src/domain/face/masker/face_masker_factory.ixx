module;
#include <memory>
#include <string>

export module domain.face.masker:factory;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

[[nodiscard]] std::unique_ptr<IFaceOccluder> create_occlusion_masker(
    const std::string& model_path,
    const foundation::ai::inference_session::Options& options =
        foundation::ai::inference_session::Options::with_best_providers());

[[nodiscard]] std::unique_ptr<IFaceRegionMasker> create_region_masker(
    const std::string& model_path,
    const foundation::ai::inference_session::Options& options =
        foundation::ai::inference_session::Options::with_best_providers());

} // namespace domain::face::masker
