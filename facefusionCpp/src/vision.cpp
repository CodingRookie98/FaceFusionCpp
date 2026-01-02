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
#include <filesystem>
#include <future>
#include <opencv2/opencv.hpp>

module vision;
import file_system;
import ffmpeg_runner;
import thread_pool;

namespace ffc::vision {
std::vector<cv::Mat> read_static_images(const std::unordered_set<std::string>& image_paths,
                                        const bool& use_thread_pool) {
    std::vector<cv::Mat> images;
    if (use_thread_pool) {
        std::vector<std::future<cv::Mat>> futures;
        for (const auto& image_path : image_paths) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([image_path]() {
                cv::Mat image = read_static_image(image_path);
                return image;
            }));
        }

        for (auto& future : futures) {
            if (cv::Mat image = future.get(); !image.empty()) {
                images.emplace_back(image);
            }
        }
    } else {
        for (const auto& image_path : image_paths) {
            if (cv::Mat image = read_static_image(image_path); !image.empty()) {
                images.emplace_back(image);
            }
        }
    }

    return images;
}

cv::Mat read_static_image(const std::string& image_path) {
    if (!is_image(image_path)) {
        throw std::invalid_argument("Path is not an image file: " + image_path);
    }
    return cv::imread(image_path, cv::IMREAD_COLOR);
}

cv::Mat resize_frame(const cv::Mat& vision_frame, const cv::Size& crop_size) {
    const int height = vision_frame.rows;
    const int width  = vision_frame.cols;
    if (height > crop_size.height || width > crop_size.width) {
        const float scale       = std::min(static_cast<float>(crop_size.height) / static_cast<float>(height), static_cast<float>(crop_size.width) / static_cast<float>(width));
        const cv::Size new_size = cv::Size(static_cast<int>(static_cast<float>(width) * scale), static_cast<int>(static_cast<float>(height) * scale));
        cv::Mat temp_image;
        cv::resize(vision_frame, temp_image, new_size);
        return temp_image;
    }
    return vision_frame.clone();
}

bool write_image(const cv::Mat& image, const std::string& image_path) {
    if (image.empty()) {
        return false;
    }
    // You may encounter long path problems in Windows
    if (cv::imwrite(image_path, image)) {
        return true;
    }
    return false;
}

cv::Size unpack_resolution(const std::string& resolution) {
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

cv::Size restrict_resolution(const cv::Size& resolution1, const cv::Size& resolution2) {
    uint64_t area1 = static_cast<uint64_t>(resolution1.width) * resolution1.height;
    uint64_t area2 = static_cast<uint64_t>(resolution2.width) * resolution2.height;
    return area1 < area2 ? resolution1 : resolution2;
}

std::tuple<std::vector<cv::Mat>, int, int> create_tile_frames(const cv::Mat& vision_frame, const std::vector<int>& size) {
    // Step 1: Initial padding
    cv::Mat padded_frame;
    copyMakeBorder(vision_frame, padded_frame, size[1], size[1], size[1], size[1], cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    // Step 2: Calculate tile width
    const int tile_width = size[0] - 2 * size[2];

    // Step 3: Calculate bottom and right padding
    const int pad_size_bottom = size[2] + tile_width - (padded_frame.rows % tile_width);
    const int pad_size_right  = size[2] + tile_width - (padded_frame.cols % tile_width);

    // Step 4: Pad the frame to make dimensions divisible by tile width
    cv::Mat fully_padded_frame;
    copyMakeBorder(padded_frame, fully_padded_frame, size[2], pad_size_bottom, size[2], pad_size_right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    int pad_height = fully_padded_frame.rows;
    int pad_width  = fully_padded_frame.cols;

    // Step 5: Define row and column ranges for tile extraction
    std::vector<cv::Mat> tile_frames;
    for (int row = size[2]; row <= pad_height - size[2] - tile_width; row += tile_width) {
        const int top = row - size[2];
        const int bottom = row + size[2] + tile_width;
        for (int col = size[2]; col <= pad_width - size[2] - tile_width; col += tile_width) {
            const int left = col - size[2];
            const int right = col + size[2] + tile_width;

            // Step 6: Extract the tile and store in the tile_frames vector
            tile_frames.push_back(fully_padded_frame(cv::Range(top, bottom), cv::Range(left, right)));
        }
    }

    // Step 7: Return the tile frames, pad width, and pad height
    return make_tuple(tile_frames, pad_width, pad_height);
}

cv::Mat merge_tile_frames(const std::vector<cv::Mat>& tile_frames, int temp_width, int temp_height,
                          int pad_width, int pad_height, const std::vector<int>& size) {
    // Step 1: Initialize the merged frame with zeros (black background)
    cv::Mat merged_frame = cv::Mat::zeros(pad_height, pad_width, CV_8UC3);

    // Step 2: Calculate the effective tile width (excluding border size[2]) and the number of tiles per row
    const int tile_width    = tile_frames[0].cols - 2 * size[2];
    const int tile_height   = tile_frames[0].rows - 2 * size[2];
    const int tiles_per_row = std::min(pad_width / tile_width, static_cast<int>(tile_frames.size()));

    // Step 3: Place each tile into the merged frame
    for (unsigned int index = 0; index < tile_frames.size(); ++index) {
        // Remove the border from each tile to get the effective region
        cv::Mat tile = tile_frames[index](cv::Rect(size[2], size[2], tile_width, tile_height));

        // Calculate the top-left position in the merged frame where the tile will be placed
        const unsigned int row_index = index / tiles_per_row;
        const unsigned int col_index = index % tiles_per_row;
        const unsigned int top       = row_index * tile_height;
        const unsigned int left      = col_index * tile_width;

        // Copy the tile to the merged frame at the computed position
        tile.copyTo(merged_frame(cv::Rect(static_cast<int>(left), static_cast<int>(top), tile_width, tile_height)));
    }

    // Step 4: Crop the merged frame to the original frame's size, removing padding
    cv::Mat final_merged_frame = merged_frame(cv::Rect(size[1], size[1], temp_width, temp_height)).clone();

    return final_merged_frame;
}

bool is_image(const std::string& path) {
    if (!file_system::is_file(path) || !file_system::file_exists(path)) {
        return false;
    }
    return cv::haveImageReader(path);
}

bool is_video(const std::string& path) {
    if (!file_system::is_file(path) || !file_system::file_exists(path)) {
        return false;
    }
    return FfmpegRunner::isVideo(path);
}

bool has_image(const std::unordered_set<std::string>& paths) {
    const bool isImg = std::ranges::all_of(paths.begin(), paths.end(), [](const std::string& path) {
        const std::string absPath = file_system::absolute_path(path);
        return is_image(path);
    });
    if (!isImg) {
        return false;
    }
    return true;
}

std::unordered_set<std::string> filter_image_paths(const std::unordered_set<std::string>& paths) {
    std::unordered_set<std::string> imagePaths;
    for (auto& path : paths) {
        if (auto absPath = file_system::absolute_path(path); is_image(absPath)) {
            imagePaths.insert(absPath);
        }
    }
    return imagePaths;
}

bool copy_image(const std::string& image_path, const std::string& destination, const cv::Size& size) {
    const cv::Mat inputImage = cv::imread(image_path, cv::IMREAD_UNCHANGED);
    if (inputImage.empty()) {
        std::cerr << "Could not open or find the image: " << image_path << std::endl;
        return false;
    }

    std::filesystem::path destinationPath = destination;
    if (!file_system::dir_exists(destinationPath.parent_path().string())) {
        file_system::create_dir(destinationPath.parent_path().string());
    }

    cv::Mat resizedImage;
    cv::Size outputSize = restrict_resolution(inputImage.size(), size);
    if (outputSize.width == 0 || outputSize.height == 0) {
        outputSize = inputImage.size();
    }

    if (outputSize.width != inputImage.size().width || outputSize.height != inputImage.size().height) {
        cv::resize(inputImage, resizedImage, outputSize);
    } else {
        if (destinationPath.extension() != ".webp") {
            file_system::copy(image_path, destinationPath.string());
            return true;
        }
        resizedImage = inputImage;
    }

    if (destinationPath.extension() == ".webp") {
        std::vector<int> compressionParams;
        compressionParams.push_back(cv::IMWRITE_WEBP_QUALITY);
        compressionParams.push_back(100);
        if (!cv::imwrite(destinationPath.string(), resizedImage, compressionParams)) {
            return false;
        }
    }

    return true;
}

bool copy_images(const std::vector<std::string>& image_paths, const std::vector<std::string>& destinations, const cv::Size& size) {
    if (image_paths.size() != destinations.size()) {
        std::cerr << __FUNCTION__ << " The number of image paths and destinations must be equal." << std::endl;
        return false;
    }
    if (image_paths.empty() || destinations.empty()) {
        std::cerr << __FUNCTION__ << " No image paths or destination paths provided." << std::endl;
        return false;
    }

    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < image_paths.size(); ++i) {
        const std::string& imagePath   = image_paths[i];
        const std::string& destination = destinations[i];
        futures.emplace_back(ThreadPool::Instance()->Enqueue([imagePath, destination, size]() {
            return copy_image(imagePath, destination, size);
        }));
    }
    for (auto& future : futures) {
        if (!future.get()) {
            return false;
        }
    }
    return true;
}

bool finalize_image(const std::string& image_path, const std::string& output_path, const cv::Size& size, const int& output_image_quality) {
    cv::Mat inputImage = cv::imread(image_path, cv::IMREAD_UNCHANGED);
    if (inputImage.empty()) {
        return false;
    }

    cv::Mat resizedImage;
    cv::Size outputSize;
    if (size.width == 0 || size.height == 0) {
        outputSize = inputImage.size();
    } else {
        outputSize = size;
    }

    if (outputSize.width != inputImage.size().width || outputSize.height != inputImage.size().height) {
        cv::resize(inputImage, resizedImage, outputSize);
    } else {
        if (output_image_quality == 100) {
            file_system::copy(image_path, output_path);
            return true;
        }
        resizedImage = inputImage;
    }

    std::vector<int> compressionParams;
    const std::string extension = std::filesystem::path(output_path).extension().string();

    if (extension == ".webp") {
        compressionParams.push_back(cv::IMWRITE_WEBP_QUALITY);
        compressionParams.push_back(std::clamp(output_image_quality, 1, 100));
    } else if (extension == ".jpg" || extension == ".jpeg") {
        compressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
        compressionParams.push_back(std::clamp(output_image_quality, 0, 100));
    } else if (extension == ".png") {
        compressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compressionParams.push_back(std::clamp((output_image_quality / 10), 0, 9));
    }

    if (!cv::imwrite(output_path, resizedImage, compressionParams)) {
        return false;
    }

    return true;
}

bool finalize_images(const std::vector<std::string>& image_paths, const std::vector<std::string>& output_paths, const cv::Size& size, const int& output_image_quality) {
    if (image_paths.size() != output_paths.size()) {
        throw std::invalid_argument("Input and output paths must have the same size");
    }

    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < image_paths.size(); ++i) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue([imagePath = image_paths[i], outputPath = output_paths[i], size, output_image_quality]() {
            try {
                return finalize_image(imagePath, outputPath, size, output_image_quality);
            } catch (const std::exception& e) {
                std::cerr << "Exception caught: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "Unknown exception caught" << std::endl;
                return false;
            }
        }));
    }

    bool allSuccess = true;
    for (auto& future : futures) {
        const bool success = future.get();
        if (!success) {
            allSuccess = false;
        }
    }

    return allSuccess;
}
} // namespace ffc::vision