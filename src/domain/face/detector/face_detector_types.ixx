module;
#include <opencv2/core/types.hpp>
#include <vector>

export module domain.face.detector:types;

export namespace domain::face::detector {
using FaceBox = cv::Rect2f;
using Landmarks = std::vector<cv::Point2f>;
using Score = float;

struct DetectionResult {
    FaceBox box;
    Landmarks landmarks;
    Score score;
};

using DetectionResults = std::vector<DetectionResult>;
} // namespace domain::face::detector
