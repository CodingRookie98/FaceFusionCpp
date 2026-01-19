module;
#include <string>
#include <vector>
#include <opencv2/core.hpp>

/**
 * @file face_swapper_api.ixx
 * @brief Face swapper interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.swapper:api;

import :types;
import domain.face;
import foundation.ai.inference_session;

export namespace domain::face::swapper {

/**
 * @brief Interface for Face Swappers
 */
class IFaceSwapper {
public:
    virtual ~IFaceSwapper() = default;

    /**
     * @brief Load the swapper model
     * @param model_path Path to the ONNX model
     * @param options Inference options (device, threads, etc.)
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief Perform face swapping
     * @param input Swap parameters including source embedding and target frame
     * @return List of processed face results (crop, matrix, etc.)
     */
    virtual std::vector<domain::face::types::FaceProcessResult> swap_face(
        const SwapInput& input) = 0;
};

} // namespace domain::face::swapper
