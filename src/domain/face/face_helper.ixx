/**
 * @file face_helper.ixx
 * @brief Utilities for face alignment, warping, and transformations
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>
#include <array>
#include <map>

export module domain.face.helper;

import domain.face;

export namespace domain::face::helper {

/**
 * @brief Warp templates for different models
 */
enum class WarpTemplateType {
    Arcface_112_v1, ///< Template for ArcFace (112x112, v1)
    Arcface_112_v2, ///< Template for ArcFace (112x112, v2)
    Arcface_128_v2, ///< Template for ArcFace (128x128, v2)
    Ffhq_512,       ///< Template for StyleGAN/FFHQ (512x512)
};

/**
 * @brief Apply Non-Maximum Suppression (NMS) to filter redundant bounding boxes
 * @param boxes List of bounding boxes
 * @param confidences List of corresponding confidence scores
 * @param nms_thresh IOU threshold for suppression
 * @return Vector of indices of kept boxes
 */
std::vector<int> apply_nms(const std::vector<cv::Rect2f>& boxes, std::vector<float> confidences,
                           float nms_thresh);

/**
 * @brief Warp a face image based on 5-point landmarks and a template
 * @param temp_vision_frame Input image frame
 * @param face_landmark_5 5-point facial landmarks
 * @param warp_template Target template points
 * @param crop_size Size of the resulting crop
 * @return Tuple of {cropped_frame, affine_matrix}
 */
std::tuple<cv::Mat, cv::Mat> warp_face_by_face_landmarks_5(
    const cv::Mat& temp_vision_frame, const types::Landmarks& face_landmark_5,
    const std::vector<cv::Point2f>& warp_template, const cv::Size& crop_size);

/**
 * @brief Warp a face image using a standard template type
 */
std::tuple<cv::Mat, cv::Mat> warp_face_by_face_landmarks_5(
    const cv::Mat& temp_vision_frame, const types::Landmarks& face_landmark_5,
    const WarpTemplateType& warp_template_type, const cv::Size& crop_size);

/**
 * @brief Estimate similarity transformation matrix from 5 landmarks
 */
cv::Mat estimate_matrix_by_face_landmark_5(const types::Landmarks& landmark_5,
                                           const std::vector<cv::Point2f>& warp_template,
                                           const cv::Size& crop_size);

/**
 * @brief Warp a face image using translation and scale parameters
 */
std::tuple<cv::Mat, cv::Mat> warp_face_by_translation(const cv::Mat& temp_vision_frame,
                                                      const std::vector<float>& translation,
                                                      const float& scale,
                                                      const cv::Size& crop_size);

/**
 * @brief Convert 68-point landmarks to 5-point landmarks
 */
types::Landmarks convert_face_landmark_68_to_5(const types::Landmarks& face_landmark_68);

/**
 * @brief Paste a processed face crop back onto the original frame using an affine matrix
 * @param temp_vision_frame Original image frame
 * @param crop_vision_frame Processed face crop
 * @param crop_mask Alpha mask for blending
 * @param affine_matrix Matrix used to warp the crop back
 * @return Merged image frame
 */
cv::Mat paste_back(const cv::Mat& temp_vision_frame, const cv::Mat& crop_vision_frame,
                   const cv::Mat& crop_mask, const cv::Mat& affine_matrix);

/**
 * @brief Create static anchor points for anchor-based detectors (e.g. YOLO, SCRFD)
 */
std::vector<std::array<int, 2>> create_static_anchors(const int& feature_stride,
                                                      const int& anchor_total,
                                                      const int& stride_height,
                                                      const int& stride_width);

/**
 * @brief Decode distance-encoded bounding box from anchor
 */
cv::Rect2f distance_2_bbox(const std::array<int, 2>& anchor, const cv::Rect2f& box);

/**
 * @brief Decode distance-encoded landmarks from anchor
 */
types::Landmarks distance_2_face_landmark_5(const std::array<int, 2>& anchor,
                                            const types::Landmarks& face_landmark_5);

/**
 * @brief Get standard warp template points for a given type
 */
std::vector<cv::Point2f> get_warp_template(const WarpTemplateType& warp_template_type);

/**
 * @brief Calculate the mathematical average of multiple face embeddings
 */
std::vector<float> calc_average_embedding(const std::vector<std::vector<float>>& embeddings);

/**
 * @brief Compute the average embedding from a list of faces and normalize it
 * @param faces List of faces
 * @return Normalized average embedding
 */
types::Embedding compute_average_embedding(const std::vector<Face>& faces);

/**
 * @brief Create a transformation matrix and target size for image rotation
 */
std::tuple<cv::Mat, cv::Size> create_rotated_mat_and_size(const double& angle,
                                                          const cv::Size& src_size);

/**
 * @brief Apply affine transformation to a bounding box
 */
cv::Rect2f transform_bbox(const cv::Rect2f& bounding_box, const cv::Mat& affine_matrix);

/**
 * @brief Apply affine transformation to a set of points
 */
std::vector<cv::Point2f> transform_points(const std::vector<cv::Point2f>& points,
                                          const cv::Mat& affine_matrix);

/**
 * @brief Linear interpolation helper
 */
std::vector<float> interp(const std::vector<float>& x, const std::vector<float>& xp,
                          const std::vector<float>& fp);

/**
 * @brief Calculate Intersection over Union (IoU) of two boxes
 */
float get_iou(const cv::Rect2f& box1, const cv::Rect2f& box2);

/**
 * @brief conditionally optimize contrast for face detection
 */
cv::Mat conditional_optimize_contrast(const cv::Mat& vision_frame);

/**
 * @brief Rotate a point back to original coordinate system
 */
cv::Point2f rotate_point_back(const cv::Point2f& pt, int angle, const cv::Size& original_size);

/**
 * @brief Rotate a bounding box back to original coordinate system
 */
cv::Rect2f rotate_box_back(const cv::Rect2f& box, int angle, const cv::Size& original_size);

/**
 * @brief Rotate an image by multiples of 90 degrees
 */
void rotate_image_90n(const cv::Mat& src, cv::Mat& dst, int angle);

/**
 * @brief Apply color matching from target crop to swapped crop (Reinhard Color Transfer in LAB
 * space)
 * @param target_crop Original face crop (reference color)
 * @param swapped_crop Swapped face crop (source color)
 * @return Color matched swapped crop
 */
cv::Mat apply_color_match(const cv::Mat& target_crop, const cv::Mat& swapped_crop);

} // namespace domain::face::helper
