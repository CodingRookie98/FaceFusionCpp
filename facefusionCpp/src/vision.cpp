/**
 ******************************************************************************
 * @file           : vision.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

module;
#include <future>
#include <opencv2/opencv.hpp>

module vision;
import file_system;
import thread_pool;

namespace ffc {
std::vector<cv::Mat> vision::readStaticImages(const std::unordered_set<std::string>& imagePaths,
                                              const bool& use_thread_pool) {
    std::vector<cv::Mat> images;
    if (use_thread_pool) {
        std::vector<std::future<cv::Mat>> futures;
        for (const auto& imagePath : imagePaths) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([imagePath]() {
                cv::Mat image = readStaticImage(imagePath);
                return image;
            }));
        }

        for (auto& future : futures) {
            if (cv::Mat image = future.get(); !image.empty()) {
                images.emplace_back(image);
            }
        }
    } else {
        for (const auto& imagePath : imagePaths) {
            if (cv::Mat image = readStaticImage(imagePath); !image.empty()) {
                images.emplace_back(image);
            }
        }
    }

    return images;
}

cv::Mat vision::readStaticImage(const std::string& imagePath) {
    //    BGR
    if (!FileSystem::isImage(imagePath)) {
        throw std::invalid_argument("Path is not an image file: " + imagePath);
    }
    return cv::imread(imagePath, cv::IMREAD_COLOR);
}

cv::Mat vision::resizeFrame(const cv::Mat& visionFrame, const cv::Size& cropSize) {
    const int height = visionFrame.rows;
    const int width = visionFrame.cols;
    cv::Mat tempImage = visionFrame.clone();
    if (height > cropSize.height || width > cropSize.width) {
        const float scale = std::min(static_cast<float>(cropSize.height) / static_cast<float>(height), static_cast<float>(cropSize.width) / static_cast<float>(width));
        const cv::Size newSize = cv::Size(static_cast<int>(static_cast<float>(width) * scale), static_cast<int>(static_cast<float>(height) * scale));
        cv::resize(visionFrame, tempImage, newSize);
    }
    return tempImage;
}

bool vision::writeImage(const cv::Mat& image, const std::string& imagePath) {
    if (image.empty()) {
        return false;
    }
    // You may encounter long path problems in Windows
    if (cv::imwrite(imagePath, image)) {
        return true;
    }
    return false;
}

cv::Size vision::unpackResolution(const std::string& resolution) {
    int width = 0;
    int height = 0;
    char delimiter = 'x';

    std::stringstream ss(resolution);
    ss >> width >> delimiter >> height;

    if (ss.fail()) {
        throw std::invalid_argument("Invalid dimensions format");
    }

    return {width, height};
}

cv::Size vision::restrictResolution(const cv::Size& resolution1, const cv::Size& resolution2) {
    uint64_t area1 = static_cast<uint64_t>(resolution1.width) * resolution1.height;
    uint64_t area2 = static_cast<uint64_t>(resolution2.width) * resolution2.height;
    return area1 < area2 ? resolution1 : resolution2;
}

std::tuple<std::vector<cv::Mat>, int, int> vision::createTileFrames(const cv::Mat& visionFrame, const std::vector<int>& size) {
    // Step 1: Initial padding
    cv::Mat paddedFrame;
    copyMakeBorder(visionFrame, paddedFrame, size[1], size[1], size[1], size[1], cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // Step 2: Calculate tile width
    const int tileWidth = size[0] - 2 * size[2];

    // Step 3: Calculate bottom and right padding
    const int padSizeBottom = size[2] + tileWidth - (paddedFrame.rows % tileWidth);
    const int padSizeRight = size[2] + tileWidth - (paddedFrame.cols % tileWidth);

    // Step 4: Pad the frame to make dimensions divisible by tile width
    cv::Mat fullyPaddedFrame;
    copyMakeBorder(paddedFrame, fullyPaddedFrame, size[2], padSizeBottom, size[2], padSizeRight, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    int padHeight = fullyPaddedFrame.rows;
    int padWidth = fullyPaddedFrame.cols;

    // Step 5: Define row and column ranges for tile extraction
    std::vector<cv::Mat> tileFrames;
    for (int row = size[2]; row <= padHeight - size[2] - tileWidth; row += tileWidth) {
        const int top = row - size[2];
        const int bottom = row + size[2] + tileWidth;
        for (int col = size[2]; col <= padWidth - size[2] - tileWidth; col += tileWidth) {
            const int left = col - size[2];
            const int right = col + size[2] + tileWidth;

            // Step 6: Extract the tile and store in the tileFrames vector
            tileFrames.push_back(fullyPaddedFrame(cv::Range(top, bottom), cv::Range(left, right)).clone());
        }
    }

    // Step 7: Return the tile frames, pad width, and pad height
    return make_tuple(tileFrames, padWidth, padHeight);
}

cv::Mat vision::mergeTileFrames(const std::vector<cv::Mat>& tileFrames, int tempWidth, int tempHeight, int padWidth, int padHeight, const std::vector<int>& size) {
    // Step 1: Initialize the merged frame with zeros (black background)
    cv::Mat mergedFrame = cv::Mat::zeros(padHeight, padWidth, CV_8UC3);

    // Step 2: Calculate the effective tile width (excluding border size[2]) and the number of tiles per row
    const int tileWidth = tileFrames[0].cols - 2 * size[2];
    const int tileHeight = tileFrames[0].rows - 2 * size[2];
    const int tilesPerRow = std::min(padWidth / tileWidth, static_cast<int>(tileFrames.size()));

    // Step 3: Place each tile into the merged frame
    for (unsigned int index = 0; index < tileFrames.size(); ++index) {
        // Remove the border from each tile to get the effective region
        cv::Mat tile = tileFrames[index](cv::Rect(size[2], size[2], tileWidth, tileHeight));

        // Calculate the top-left position in the merged frame where the tile will be placed
        const unsigned int rowIndex = index / tilesPerRow;
        const unsigned int colIndex = index % tilesPerRow;
        const unsigned int top = rowIndex * tileHeight;
        const unsigned int left = colIndex * tileWidth;

        // Copy the tile to the merged frame at the computed position
        tile.copyTo(mergedFrame(cv::Rect(static_cast<int>(left), static_cast<int>(top), tileWidth, tileHeight)));
    }

    // Step 4: Crop the merged frame to the original frame's size, removing padding
    cv::Mat finalMergedFrame = mergedFrame(cv::Rect(size[1], size[1], tempWidth, tempHeight)).clone();

    return finalMergedFrame;
}
} // namespace ffc