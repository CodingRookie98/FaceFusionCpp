/**
 ******************************************************************************
 * @file           : vision.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-6
 ******************************************************************************
 */

#include "vision.h"
#include <thread_pool/thread_pool.h>
#include "file_system.h"

namespace Ffc {
std::vector<cv::Mat> Vision::readStaticImages(const std::vector<std::string> &imagePaths) {
    std::vector<cv::Mat> images;
    for (const auto &imagePath : imagePaths) {
        cv::Mat image = readStaticImage(imagePath);
        if (!image.empty()) {
            images.emplace_back(image);
        }
    }

    return images;
}

std::vector<cv::Mat> Vision::multiReadStaticImages(const std::unordered_set<std::string> &imagePaths) {
    std::vector<cv::Mat> images;
    dp::thread_pool pool(std::thread::hardware_concurrency());
    std::vector<std::future<cv::Mat>> futures;
    for (const auto &imagePath : imagePaths) {
        futures.emplace_back(pool.enqueue([imagePath]() {
            cv::Mat image = readStaticImage(imagePath);
            return image;
        }));
    }

    for (auto &future : futures) {
        cv::Mat image = future.get();
        if (!image.empty()) {
            images.emplace_back(image);
        }
    }

    return images;
}

cv::Mat Vision::readStaticImage(const std::string &imagePath) {
    //    BGR
    if (FileSystem::fileExists(imagePath) && FileSystem::isFile(imagePath) && FileSystem::isImage(imagePath)) {
        return cv::imread(imagePath, cv::IMREAD_COLOR);
    } else if (!FileSystem::fileExists(imagePath)) {
        throw std::invalid_argument("File does not exist: " + imagePath);
    } else if (!FileSystem::isFile(imagePath)) {
        throw std::invalid_argument("Path is not a file: " + imagePath);
    } else if (!FileSystem::isImage(imagePath)) {
        throw std::invalid_argument("Path is not an image file: " + imagePath);
    } else {
        throw std::invalid_argument("Unknown error occurred while reading image: " + imagePath);
    }
}

cv::Mat Vision::resizeFrameResolution(const cv::Mat &visionFrame, const cv::Size &cropSize) {
    const int height = visionFrame.rows;
    const int width = visionFrame.cols;
    cv::Mat tempImage = visionFrame.clone();
    if (height > cropSize.height || width > cropSize.width) {
        const float scale = std::min((float)cropSize.height / height, (float)cropSize.width / width);
        cv::Size newSize = cv::Size(int(width * scale), int(height * scale));
        cv::resize(visionFrame, tempImage, newSize);
    }
    return tempImage;
}

bool Vision::writeImage(const cv::Mat &image, const std::string &imagePath) {
    if (image.empty()) {
        return false;
    }
    // You may encounter long path problems in Windows
    if (cv::imwrite(imagePath, image)) {
        return true;
    }
    return false;
}

cv::Size Vision::unpackResolution(const std::string &resolution) {
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

std::vector<cv::Mat> Vision::readStaticImages(const std::unordered_set<std::string> &imagePaths) {
    std::vector<cv::Mat> images;
    for (const auto &imagePath : imagePaths) {
        cv::Mat image = readStaticImage(imagePath);
        if (!image.empty()) {
            images.emplace_back(image);
        }
    }

    return images;
}

cv::Size Vision::restrictResolution(const cv::Size &resolution1, const cv::Size &resolution2) {
    uint64_t area1 = static_cast<uint64_t>(resolution1.width) * resolution1.height;
    uint64_t area2 = static_cast<uint64_t>(resolution2.width) * resolution2.height;
    return area1 < area2 ? resolution1 : resolution2;
}

std::tuple<std::vector<cv::Mat>, int, int> Vision::createTileFrames(const cv::Mat &visionFrame, const std::vector<int> &size) {
    // Step 1: Initial padding
    cv::Mat paddedFrame;
    copyMakeBorder(visionFrame, paddedFrame, size[1], size[1], size[1], size[1], cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // Step 2: Calculate tile width
    int tileWidth = size[0] - 2 * size[2];

    // Step 3: Calculate bottom and right padding
    int padSizeBottom = size[2] + tileWidth - (paddedFrame.rows % tileWidth);
    int padSizeRight = size[2] + tileWidth - (paddedFrame.cols % tileWidth);

    // Step 4: Pad the frame to make dimensions divisible by tile width
    cv::Mat fullyPaddedFrame;
    copyMakeBorder(paddedFrame, fullyPaddedFrame, size[2], padSizeBottom, size[2], padSizeRight, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    int padHeight = fullyPaddedFrame.rows;
    int padWidth = fullyPaddedFrame.cols;

    // Step 5: Define row and column ranges for tile extraction
    std::vector<cv::Mat> tileFrames;
    for (int row = size[2]; row <= padHeight - size[2] - tileWidth; row += tileWidth) {
        int top = row - size[2];
        int bottom = row + size[2] + tileWidth;
        for (int col = size[2]; col <= padWidth - size[2] - tileWidth; col += tileWidth) {
            int left = col - size[2];
            int right = col + size[2] + tileWidth;

            // Step 6: Extract the tile and store in the tileFrames vector
            tileFrames.push_back(fullyPaddedFrame(cv::Range(top, bottom), cv::Range(left, right)).clone());
        }
    }

    // Step 7: Return the tile frames, pad width, and pad height
    return make_tuple(tileFrames, padWidth, padHeight);
}

cv::Mat Vision::mergeTileFrames(const std::vector<cv::Mat> &tileFrames, int tempWidth, int tempHeight, int padWidth, int padHeight, const std::vector<int> &size) {
    // Step 1: Initialize the merged frame with zeros (black background)
    cv::Mat mergedFrame = cv::Mat::zeros(padHeight, padWidth, CV_8UC3);

    // Step 2: Calculate the effective tile width (excluding border size[2]) and the number of tiles per row
    int tileWidth = tileFrames[0].cols - 2 * size[2];
    int tileHeight = tileFrames[0].rows - 2 * size[2];
    int tilesPerRow = std::min(padWidth / tileWidth, (int)tileFrames.size());

    // Step 3: Place each tile into the merged frame
    for (size_t index = 0; index < tileFrames.size(); ++index) {
        // Remove the border from each tile to get the effective region
       cv::Mat tile = tileFrames[index](cv::Rect(size[2], size[2], tileWidth, tileHeight));

        // Calculate the top-left position in the merged frame where the tile will be placed
        int rowIndex = index / tilesPerRow;
        int colIndex = index % tilesPerRow;
        int top = rowIndex * tileHeight;
        int left = colIndex * tileWidth;

        // Copy the tile to the merged frame at the computed position
        tile.copyTo(mergedFrame(cv::Rect(left, top, tileWidth, tileHeight)));
    }

    // Step 4: Crop the merged frame to the original frame's size, removing padding
    cv::Mat finalMergedFrame = mergedFrame(cv::Rect(size[1], size[1], tempWidth, tempHeight)).clone();

    return finalMergedFrame;
}
} // namespace Ffc