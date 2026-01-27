/**
 * @file vision.ixx
 * @brief Computer vision utility module
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <tuple>

export module foundation.media.vision;

namespace foundation::media::vision {

/**
 * @brief Read a single static image from disk
 * @param image_path Path to the image file
 * @return cv::Mat containing the image data
 */
export cv::Mat read_static_image(const std::string& image_path);

/**
 * @brief Read multiple static images from disk
 * @param image_paths Set of paths to image files
 * @param use_thread_pool Whether to use a thread pool for parallel loading
 * @return Vector of loaded images
 */
export std::vector<cv::Mat> read_static_images(const std::unordered_set<std::string>& image_paths,
                                               const bool& use_thread_pool = true);

/**
 * @brief Resize an image frame to specified dimensions
 * @param vision_frame Input image
 * @param crop_size Target size
 * @return Resized image
 */
export cv::Mat resize_frame(const cv::Mat& vision_frame, const cv::Size& crop_size);

/**
 * @brief Write an image to disk
 * @param image Image data to write
 * @param image_path Target file path
 * @return true if successful
 */
export bool write_image(const cv::Mat& image, const std::string& image_path);

/**
 * @brief Parse a resolution string (e.g., "1280x720") into a cv::Size
 * @param resolution String representation of resolution
 * @return Parsed cv::Size
 */
export cv::Size unpack_resolution(const std::string& resolution);

/**
 * @brief Restrict resolution1 to be no larger than resolution2 while maintaining aspect ratio
 */
export cv::Size restrict_resolution(const cv::Size& resolution1, const cv::Size& resolution2);

/**
 * @brief Split a frame into overlapping tiles for processing
 * @param vision_frame Input image
 * @param size Tile dimensions
 * @return Tuple of {tiles, temp_width, temp_height}
 */
export std::tuple<std::vector<cv::Mat>, int, int> create_tile_frames(const cv::Mat& vision_frame,
                                                                     const std::vector<int>& size);

/**
 * @brief Merge processed tiles back into a single frame
 */
export cv::Mat merge_tile_frames(const std::vector<cv::Mat>& tile_frames, int temp_width,
                                 int temp_height, int pad_width, int pad_height,
                                 const std::vector<int>& size);

/**
 * @brief Check if a path points to a supported image file
 */
export bool is_image(const std::string& path);

/**
 * @brief Check if a path points to a supported video file
 */
export bool is_video(const std::string& path);

/**
 * @brief Check if any of the paths point to valid images
 */
export bool has_image(const std::unordered_set<std::string>& paths);

/**
 * @brief Filter a set of paths, returning only those that are valid images
 */
export std::unordered_set<std::string> filter_image_paths(
    const std::unordered_set<std::string>& paths);

/**
 * @brief Copy and optionally resize an image
 */
export bool copy_image(const std::string& image_path, const std::string& destination,
                       const cv::Size& size = cv::Size(0, 0));

/**
 * @brief Copy and optionally resize multiple images
 */
export bool copy_images(const std::vector<std::string>& image_paths,
                        const std::vector<std::string>& destinations,
                        const cv::Size& size = cv::Size(0, 0));

/**
 * @brief Perform final processing (resize, quality) and save image
 */
export bool finalize_image(const std::string& image_path, const std::string& output_path,
                           const cv::Size& size = cv::Size(0, 0),
                           const int& output_image_quality = 100);

/**
 * @brief Finalize multiple images
 */
export bool finalize_images(const std::vector<std::string>& image_paths,
                            const std::vector<std::string>& output_paths,
                            const cv::Size& size = cv::Size(0, 0),
                            const int& output_image_quality = 100);

} // namespace foundation::media::vision
