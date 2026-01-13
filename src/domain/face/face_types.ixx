module;
#include <vector>
#include <opencv2/opencv.hpp>

export module domain.face:types;

export namespace domain::face::types {
using Embedding = std::vector<float>;
using Landmarks = std::vector<cv::Point2f>;
using Score = float;
} // namespace domain::face::types
