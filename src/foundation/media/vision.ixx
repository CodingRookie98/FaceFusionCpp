/**
 ******************************************************************************
 * @file           : vision.ixx
 * @brief          : Vision module interface
 ******************************************************************************
 */

module;
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <tuple>

export module foundation.media.vision;

export namespace foundation::media::vision {
export cv::Mat read_static_image(const std::string& image_path);

export std::vector<cv::Mat> read_static_images(const std::unordered_set<std::string>& image_paths,
                                               const bool& use_thread_pool = true);

export cv::Mat resize_frame(const cv::Mat& vision_frame, const cv::Size& crop_size);

export bool write_image(const cv::Mat& image, const std::string& image_path);

export cv::Size unpack_resolution(const std::string& resolution);

export cv::Size restrict_resolution(const cv::Size& resolution1, const cv::Size& resolution2);

export std::tuple<std::vector<cv::Mat>, int, int> create_tile_frames(const cv::Mat& vision_frame,
                                                                     const std::vector<int>& size);

export cv::Mat merge_tile_frames(const std::vector<cv::Mat>& tile_frames, int temp_width,
                                 int temp_height, int pad_width, int pad_height,
                                 const std::vector<int>& size);

export bool is_image(const std::string& path);

export bool is_video(const std::string& path);

export bool has_image(const std::unordered_set<std::string>& paths);

export std::unordered_set<std::string> filter_image_paths(
    const std::unordered_set<std::string>& paths);

export bool copy_image(const std::string& image_path, const std::string& destination,
                       const cv::Size& size = cv::Size(0, 0));

export bool copy_images(const std::vector<std::string>& image_paths,
                        const std::vector<std::string>& destinations,
                        const cv::Size& size = cv::Size(0, 0));

export bool finalize_image(const std::string& image_path, const std::string& output_path,
                           const cv::Size& size = cv::Size(0, 0),
                           const int& output_image_quality = 100);

export bool finalize_images(const std::vector<std::string>& image_paths,
                            const std::vector<std::string>& output_paths,
                            const cv::Size& size = cv::Size(0, 0),
                            const int& output_image_quality = 100);
} // namespace foundation::media::vision
