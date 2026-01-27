/**
 * @file face_landmarker_factory.ixx
 * @brief Factory for creating Face Landmarker instances
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>

export module domain.face.landmarker:factory;
import :api;

export namespace domain::face::landmarker {

/**
 * @brief Available types of Face Landmarkers
 */
enum class LandmarkerType {
    _2DFAN,    ///< 2DFAN4 model (68 points)
    Peppawutz, ///< Peppawutz model (68 points)
    _68By5     ///< Predict 68 points from 5 facial landmarks
};

/**
 * @brief Create a Landmarker instance
 * @param type Type of Landmarker to create
 * @return std::unique_ptr<IFaceLandmarker> Created landmarker instance
 */
std::unique_ptr<IFaceLandmarker> create_landmarker(LandmarkerType type);

} // namespace domain::face::landmarker
