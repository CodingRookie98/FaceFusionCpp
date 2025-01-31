/**
 ******************************************************************************
 * @file           : vision.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

module;
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>

export module vision;

namespace ffc {
export namespace Vision {
cv::Mat readStaticImage(const std::string &imagePath);
std::vector<cv::Mat> readStaticImages(const std::unordered_set<std::string> &imagePaths,
                                      unsigned short threadCnt = 1);
cv::Mat resizeFrame(const cv::Mat &visionFrame, const cv::Size &cropSize);
bool writeImage(const cv::Mat &image, const std::string &imagePath);
cv::Size unpackResolution(const std::string &resolution);
cv::Size restrictResolution(const cv::Size &resolution1, const cv::Size &resolution2);
std::tuple<std::vector<cv::Mat>, int, int>
createTileFrames(const cv::Mat &visionFrame, const std::vector<int> &size);
cv::Mat mergeTileFrames(const std::vector<cv::Mat> &tileFrames, int tempWidth, int tempHeight,
                        int padWidth, int padHeight, const std::vector<int> &size);
}

} // namespace ffc::Vision
