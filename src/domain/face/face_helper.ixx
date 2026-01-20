/**
 ******************************************************************************
 * @file           : face_helper.ixx
 * @brief          : Face helper module interface
 ******************************************************************************
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

enum class WarpTemplateType {
    Arcface_112_v1,
    Arcface_112_v2,
    Arcface_128_v2,
    Ffhq_512,
};

std::vector<int> apply_nms(const std::vector<cv::Rect2f>& boxes, std::vector<float> confidences,
                           float nms_thresh);

// return: 0->croped_vision_frame, 1->affine_matrix
std::tuple<cv::Mat, cv::Mat> warp_face_by_face_landmarks_5(
    const cv::Mat& temp_vision_frame, const types::Landmarks& face_landmark_5,
    const std::vector<cv::Point2f>& warp_template, const cv::Size& crop_size);

std::tuple<cv::Mat, cv::Mat> warp_face_by_face_landmarks_5(
    const cv::Mat& temp_vision_frame, const types::Landmarks& face_landmark_5,
    const WarpTemplateType& warp_template_type, const cv::Size& crop_size);

cv::Mat estimate_matrix_by_face_landmark_5(const types::Landmarks& landmark_5,
                                           const std::vector<cv::Point2f>& warp_template,
                                           const cv::Size& crop_size);

std::tuple<cv::Mat, cv::Mat> warp_face_by_translation(const cv::Mat& temp_vision_frame,
                                                      const std::vector<float>& translation,
                                                      const float& scale,
                                                      const cv::Size& crop_size);

types::Landmarks convert_face_landmark_68_to_5(const types::Landmarks& face_landmark_68);

cv::Mat paste_back(const cv::Mat& temp_vision_frame, const cv::Mat& crop_vision_frame,
                   const cv::Mat& crop_mask, const cv::Mat& affine_matrix);

std::vector<std::array<int, 2>> create_static_anchors(const int& feature_stride,
                                                      const int& anchor_total,
                                                      const int& stride_height,
                                                      const int& stride_width);

cv::Rect2f distance_2_bbox(const std::array<int, 2>& anchor, const cv::Rect2f& box);

types::Landmarks distance_2_face_landmark_5(const std::array<int, 2>& anchor,
                                            const types::Landmarks& face_landmark_5);

std::vector<cv::Point2f> get_warp_template(const WarpTemplateType& warp_template_type);

std::vector<float> calc_average_embedding(const std::vector<std::vector<float>>& embeddings);

std::tuple<cv::Mat, cv::Size> create_rotated_mat_and_size(const double& angle,
                                                          const cv::Size& src_size);

cv::Rect2f transform_bbox(const cv::Rect2f& bounding_box, const cv::Mat& affine_matrix);

std::vector<cv::Point2f> transform_points(const std::vector<cv::Point2f>& points,
                                          const cv::Mat& affine_matrix);

std::vector<float> interp(const std::vector<float>& x, const std::vector<float>& xp,
                          const std::vector<float>& fp);

float get_iou(const cv::Rect2f& box1, const cv::Rect2f& box2);

cv::Mat conditional_optimize_contrast(const cv::Mat& vision_frame);

// Rotation helpers
cv::Point2f rotate_point_back(const cv::Point2f& pt, int angle, const cv::Size& original_size);
cv::Rect2f rotate_box_back(const cv::Rect2f& box, int angle, const cv::Size& original_size);
void rotate_image_90n(const cv::Mat& src, cv::Mat& dst, int angle);

} // namespace domain::face::helper
