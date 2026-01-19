module;
#include <opencv2/core/types.hpp>
#include <vector>

export module domain.face.detector:types;

export namespace domain::face::detector {
/**
 * @brief Detected face bounding box
 */
using FaceBox = cv::Rect2f;

/**
 * @brief Face landmarks (5 points)
 */
using Landmarks = std::vector<cv::Point2f>;

/**
 * @brief Detection confidence score
 */
using Score = float;

/**
 * @brief Result of a single face detection
 */
struct DetectionResult {
    /// @brief Bounding box of the face
    FaceBox box;
    /// @brief 5-point landmarks
    Landmarks landmarks;
    /// @brief Confidence score (0.0 - 1.0)
    Score score;
};

/**
 * @brief List of detection results
 */
using DetectionResults = std::vector<DetectionResult>;
} // namespace domain::face::detector
