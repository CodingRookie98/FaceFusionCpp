/**
 ******************************************************************************
 * @file           : vision.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_VISION_H_
#define FACEFUSIONCPP_SRC_VISION_H_

#include <unordered_set>
#include <opencv2/opencv.hpp>

namespace Ffc {

class Vision {
public:
    static std::vector<cv::Mat> readStaticImages(const std::vector<std::string> &imagePaths);
    static cv::Mat readStaticImage(const std::string &imagePath);
    static std::vector<cv::Mat> readStaticImages(const std::unordered_set<std::string> &imagePaths);
    static std::vector<cv::Mat> multiReadStaticImages(const std::unordered_set<std::string> &imagePaths);
    static cv::Mat resizeFrameResolution(const cv::Mat &visionFrame, const cv::Size &cropSize);
    static bool writeImage(const cv::Mat &image, const std::string &imagePath);
    static cv::Size unpackResolution(const std::string &resolution);
    static cv::Size restrictResolution(const cv::Size &resolution1, const cv::Size &resolution2);
    static std::tuple<std::vector<cv::Mat>, int, int>
    createTileFrames(const cv::Mat &visionFrame, const std::vector<int> &size);
    static cv::Mat mergeTileFrames(const std::vector<cv::Mat> &tileFrames, int tempWidth, int tempHeight,
                                   int padWidth, int padHeight, const std::vector<int> &size);
};

} // namespace Ffc

#endif // FACEFUSIONCPP_SRC_VISION_H_
