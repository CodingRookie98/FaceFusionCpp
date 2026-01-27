/**
 * @file face_masker_factory.ixx
 * @brief Factory for creating Face Masker and Occluder instances
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>
#include <string>

export module domain.face.masker:factory;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

/**
 * @brief Create an instance of an occlusion masker
 * @param model_path Path to the model file
 * @param options Inference session options
 * @return std::unique_ptr<IFaceOccluder> Created occlusion masker instance
 */
[[nodiscard]] std::unique_ptr<IFaceOccluder> create_occlusion_masker(
    const std::string& model_path,
    const foundation::ai::inference_session::Options& options =
        foundation::ai::inference_session::Options::with_best_providers());

/**
 * @brief Create an instance of a region masker (Face Parsing)
 * @param model_path Path to the model file
 * @param options Inference session options
 * @return std::unique_ptr<IFaceRegionMasker> Created region masker instance
 */
[[nodiscard]] std::unique_ptr<IFaceRegionMasker> create_region_masker(
    const std::string& model_path,
    const foundation::ai::inference_session::Options& options =
        foundation::ai::inference_session::Options::with_best_providers());

} // namespace domain::face::masker
