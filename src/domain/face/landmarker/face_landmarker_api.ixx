module;
#include <opencv2/core.hpp>
#include <string>
#include <vector>

/**
 * @file face_landmarker_api.ixx
 * @brief Face landmarker interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.landmarker:api;
import domain.face;
import foundation.ai.inference_session;

export namespace domain::face::landmarker {

/**
 * @brief Face landmarker result structure
 */
struct LandmarkerResult {
    domain::face::types::Landmarks landmarks; ///< Detected landmarks
    float score;                              ///< Confidence score
};

/**
 * @brief Interface for Face Landmarkers
 */
class IFaceLandmarker {
public:
    virtual ~IFaceLandmarker() = default;

    /**
     * @brief Load the landmarker model
     * @param model_path Path to the model file
     * @param options Inference session options
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief Detect face landmarks
     * @param image Input image
     * @param bbox Bounding box of the face
     * @return LandmarkerResult containing landmarks and score
     */
    virtual LandmarkerResult detect(const cv::Mat& image, const cv::Rect2f& bbox) = 0;

    /**
     * @brief Expand 5-point landmarks to 68-point (Helper)
     * @param landmarks5 Input 5-point landmarks
     * @return 68-point landmarks (interpolated or predicted)
     */
    virtual domain::face::types::Landmarks expand_68_from_5(
        const domain::face::types::Landmarks& landmarks5) {
        return {};
    }
};

} // namespace domain::face::landmarker
