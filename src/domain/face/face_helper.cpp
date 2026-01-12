/**
 ******************************************************************************
 * @file           : face_helper.cpp
 * @brief          : Face helper module implementation
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <limits>
#include <algorithm>
#include <numeric>
#include <vector>
#include <tuple>
#include <map>
#include <memory>
#include <array>

module domain.face.helper;

import domain.face;

namespace domain::face::helper {

    float get_iou(const cv::Rect2f& box1, const cv::Rect2f& box2) {
        const float x1 = std::max(box1.x, box2.x);
        const float y1 = std::max(box1.y, box2.y);
        const float x2 = std::min(box1.x + box1.width, box2.x + box2.width);
        const float y2 = std::min(box1.y + box1.height, box2.y + box2.height);
        const float w = std::max(0.f, x2 - x1);
        const float h = std::max(0.f, y2 - y1);
        const float over_area = w * h;
        if (over_area == 0)
            return 0.0;
        const float union_area = box1.area() + box2.area() - over_area;
        return over_area / union_area;
    }

    std::vector<int> apply_nms(const std::vector<cv::Rect2f>& boxes,
                               std::vector<float> confidences,
                               const float nms_thresh) {
        std::vector<size_t> indices(confidences.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::ranges::sort(indices, [&confidences](const size_t index1, const size_t index2) {
            return confidences[index1] > confidences[index2];
        });

        const size_t num_box = confidences.size();
        std::vector<bool> is_suppressed(num_box, false);
        for (size_t i = 0; i < num_box; ++i) {
            size_t idx_i = indices[i];
            if (is_suppressed[idx_i]) {
                continue;
            }
            for (size_t j = i + 1; j < num_box; ++j) {
                size_t idx_j = indices[j];
                if (is_suppressed[idx_j]) {
                    continue;
                }

                if (const float ovr = get_iou(boxes[idx_i], boxes[idx_j]); ovr > nms_thresh) {
                    is_suppressed[idx_j] = true;
                }
            }
        }

        std::vector<int> keep_inds;
        for (size_t i = 0; i < num_box; ++i) {
            size_t idx = indices[i];
            if (!is_suppressed[idx]) {
                keep_inds.emplace_back(static_cast<int>(idx));
            }
        }
        return keep_inds;
    }

    std::tuple<cv::Mat, cv::Mat>
    warp_face_by_face_landmarks_5(const cv::Mat& temp_vision_frame,
                                  const types::Landmarks& face_landmark_5,
                                  const std::vector<cv::Point2f>& warp_template,
                                  const cv::Size& crop_size) {
        cv::Mat affine_matrix = estimate_matrix_by_face_landmark_5(face_landmark_5, warp_template, crop_size);
        cv::Mat crop_vision;
        cv::warpAffine(temp_vision_frame, crop_vision, affine_matrix, crop_size, cv::INTER_AREA, cv::BORDER_REPLICATE);
        return std::make_tuple(crop_vision, affine_matrix);
    }

    std::tuple<cv::Mat, cv::Mat> warp_face_by_face_landmarks_5(const cv::Mat& temp_vision_frame,
                                                               const types::Landmarks& face_landmark_5,
                                                               const WarpTemplateType& warp_template_type,
                                                               const cv::Size& crop_size) {
        const std::vector<cv::Point2f> warp_template = get_warp_template(warp_template_type);
        return warp_face_by_face_landmarks_5(temp_vision_frame, face_landmark_5, warp_template, crop_size);
    }

    cv::Mat estimate_matrix_by_face_landmark_5(const types::Landmarks& landmark_5,
                                               const std::vector<cv::Point2f>& warp_template,
                                               const cv::Size& crop_size) {
        std::vector<cv::Point2f> normed_warp_template = warp_template;
        for (auto& point : normed_warp_template) {
            point.x *= static_cast<float>(crop_size.width);
            point.y *= static_cast<float>(crop_size.height);
        }
        // types::Landmarks is std::vector<cv::Point2f>, compatible with estimateAffinePartial2D
        cv::Mat affine_matrix = cv::estimateAffinePartial2D(landmark_5, normed_warp_template,
                                                            cv::noArray(), cv::RANSAC, 100);
        return affine_matrix;
    }

    std::tuple<cv::Mat, cv::Mat>
    warp_face_by_translation(const cv::Mat& temp_vision_frame,
                             const std::vector<float>& translation,
                             const float& scale, const cv::Size& crop_size) {
        cv::Mat affine_matrix = (cv::Mat_<float>(2, 3) << scale, 0.f, translation[0], 0.f, scale, translation[1]);
        cv::Mat crop_img;
        warpAffine(temp_vision_frame, crop_img, affine_matrix, crop_size);
        return std::make_tuple(crop_img, affine_matrix);
    }

    types::Landmarks
    convert_face_landmark_68_to_5(const types::Landmarks& face_landmark_68) {
        types::Landmarks face_landmark_5_68(5);
        float x = 0, y = 0;
        // left_eye (36-41)
        for (int i = 36; i < 42; i++) {
            x += face_landmark_68[i].x;
            y += face_landmark_68[i].y;
        }
        x /= 6;
        y /= 6;
        face_landmark_5_68[0] = cv::Point2f(x, y);

        x = 0, y = 0;
        // right_eye (42-47)
        for (int i = 42; i < 48; i++) {
            x += face_landmark_68[i].x;
            y += face_landmark_68[i].y;
        }
        x /= 6;
        y /= 6;
        face_landmark_5_68[1] = cv::Point2f(x, y);

        face_landmark_5_68[2] = face_landmark_68[30]; // nose
        face_landmark_5_68[3] = face_landmark_68[48]; // left_mouth_end
        face_landmark_5_68[4] = face_landmark_68[54]; // right_mouth_end

        return face_landmark_5_68;
    }

    cv::Mat paste_back(const cv::Mat& temp_vision_frame, const cv::Mat& crop_vision_frame,
                       const cv::Mat& crop_mask, const cv::Mat& affine_matrix) {
        cv::Mat inverse_matrix;
        cv::invertAffineTransform(affine_matrix, inverse_matrix);
        cv::Mat inverse_mask;
        const cv::Size temp_size(temp_vision_frame.cols, temp_vision_frame.rows);
        warpAffine(crop_mask, inverse_mask, inverse_matrix, temp_size);
        inverse_mask.setTo(0, inverse_mask < 0);
        inverse_mask.setTo(1, inverse_mask > 1);
        cv::Mat inverse_vision_frame;
        warpAffine(crop_vision_frame, inverse_vision_frame, inverse_matrix, temp_size, cv::INTER_LINEAR, cv::BORDER_REPLICATE);

        std::vector<cv::Mat> inverse_vision_frame_bgrs(3);
        split(inverse_vision_frame, inverse_vision_frame_bgrs);
        std::vector<cv::Mat> temp_vision_frame_bgrs(3);
        split(temp_vision_frame, temp_vision_frame_bgrs);
        for (int c = 0; c < 3; c++) {
            inverse_vision_frame_bgrs[c].convertTo(inverse_vision_frame_bgrs[c], CV_32FC1);
            temp_vision_frame_bgrs[c].convertTo(temp_vision_frame_bgrs[c], CV_32FC1);
        }
        std::vector<cv::Mat> channel_mats(3);

        channel_mats[0] = inverse_mask.mul(inverse_vision_frame_bgrs[0]) + temp_vision_frame_bgrs[0].mul(1 - inverse_mask);
        channel_mats[1] = inverse_mask.mul(inverse_vision_frame_bgrs[1]) + temp_vision_frame_bgrs[1].mul(1 - inverse_mask);
        channel_mats[2] = inverse_mask.mul(inverse_vision_frame_bgrs[2]) + temp_vision_frame_bgrs[2].mul(1 - inverse_mask);

        cv::Mat paste_vision_frame;
        merge(channel_mats, paste_vision_frame);
        paste_vision_frame.convertTo(paste_vision_frame, CV_8UC3);
        return paste_vision_frame;
    }

    std::vector<std::array<int, 2>>
    create_static_anchors(const int& feature_stride, const int& anchor_total,
                          const int& stride_height, const int& stride_width) {
        std::vector<std::array<int, 2>> anchors;
        // Create a grid of (y, x) coordinates
        for (int i = 0; i < stride_height; ++i) {
            for (int j = 0; j < stride_width; ++j) {
                // Compute the original image coordinates
                const int y = i * feature_stride;
                const int x = j * feature_stride;

                // Add each anchor for the current grid point
                for (int k = 0; k < anchor_total; ++k) {
                    anchors.push_back({y, x});
                }
            }
        }
        return anchors;
    }

    cv::Rect2f distance_2_bbox(const std::array<int, 2>& anchor, const cv::Rect2f& bbox) {
        cv::Rect2f result;
        const float anchor_x = static_cast<float>(anchor[1]);
        const float anchor_y = static_cast<float>(anchor[0]);
        result.x = anchor_x - bbox.x;
        result.y = anchor_y - bbox.y;
        result.width = bbox.x + bbox.width;
        result.height = bbox.y + bbox.height;
        return result;
    }

    types::Landmarks
    distance_2_face_landmark_5(const std::array<int, 2>& anchor, const types::Landmarks& face_landmark_5) {
        types::Landmarks face_landmark_5_ = face_landmark_5; // copy
        for (int i = 0; i < 5; ++i) {
            face_landmark_5_[i].x += static_cast<float>(anchor[1]);
            face_landmark_5_[i].y += static_cast<float>(anchor[0]);
        }
        return face_landmark_5_;
    }

    std::vector<cv::Point2f> get_warp_template(const WarpTemplateType& warp_template_type) {
        static const std::map<WarpTemplateType, std::vector<cv::Point2f>> warp_templates = {
            {WarpTemplateType::Arcface_112_v1, {{0.35473214f, 0.45658929f}, {0.64526786f, 0.45658929f}, {0.50000000f, 0.61154464f}, {0.37913393f, 0.77687500f}, {0.62086607f, 0.77687500f}}},
            {WarpTemplateType::Arcface_112_v2, {{0.34191607f, 0.46157411f}, {0.65653393f, 0.45983393f}, {0.50022500f, 0.64050536f}, {0.37097589f, 0.82469196f}, {0.63151696f, 0.82325089f}}},
            {WarpTemplateType::Arcface_128_v2, {{0.36167656f, 0.40387734f}, {0.63696719f, 0.40235469f}, {0.50019687f, 0.56044219f}, {0.38710391f, 0.72160547f}, {0.61507734f, 0.72034453f}}},
            {WarpTemplateType::Ffhq_512, {{0.37691676f, 0.46864664f}, {0.62285697f, 0.46912813f}, {0.50123859f, 0.61331904f}, {0.39308822f, 0.72541100f}, {0.61150205f, 0.72490465f}}}
        };
        return warp_templates.at(warp_template_type);
    }

    std::vector<float> calc_average_embedding(const std::vector<std::vector<float>>& embeddings) {
        if (embeddings.empty()) return {};

        std::vector<float> average_embedding(embeddings[0].size(), 0.0);
        for (const auto& embedding : embeddings) {
            for (size_t i = 0; i < embedding.size(); ++i) {
                average_embedding[i] += embedding[i];
            }
        }
        for (float& value : average_embedding) {
            value /= static_cast<float>(embeddings.size());
        }
        return average_embedding;
    }

    std::tuple<cv::Mat, cv::Size> create_rotated_mat_and_size(const double& angle, const cv::Size& src_size) {
        cv::Mat rotated_mat = cv::getRotationMatrix2D(cv::Point2f(static_cast<float>(src_size.width) / 2.f, static_cast<float>(src_size.height) / 2.f), angle, 1.0);
        const cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src_size, static_cast<float>(angle)).boundingRect2f();
        rotated_mat.at<double>(0, 2) += (bbox.width - static_cast<float>(src_size.width)) * 0.5;
        rotated_mat.at<double>(1, 2) += (bbox.height - static_cast<float>(src_size.height)) * 0.5;
        cv::Size rotated_size(static_cast<int>(bbox.width), static_cast<int>(bbox.height));
        return std::make_tuple(rotated_mat, rotated_size);
    }

    std::vector<cv::Point2f> transform_points(const std::vector<cv::Point2f>& points, const cv::Mat& affine_matrix) {
        std::vector<cv::Point2f> transformed_points;
        cv::transform(points, transformed_points, affine_matrix);
        return transformed_points;
    }

    cv::Rect2f transform_bbox(const cv::Rect2f& bbox, const cv::Mat& affine_matrix) {
        const std::vector<cv::Point2f> points = {
            {bbox.x, bbox.y},
            {bbox.x + bbox.width, bbox.y},
            {bbox.x, bbox.y + bbox.height},
            {bbox.x + bbox.width, bbox.y + bbox.height}
        };
        const std::vector<cv::Point2f> transformed_points = transform_points(points, affine_matrix);

        float new_x_min = std::numeric_limits<float>::max();
        float new_y_min = std::numeric_limits<float>::max();
        float new_x_max = std::numeric_limits<float>::min();
        float new_y_max = std::numeric_limits<float>::min();

        for (const auto& point : transformed_points) {
            new_x_min = std::min(new_x_min, point.x);
            new_y_min = std::min(new_y_min, point.y);
            new_x_max = std::max(new_x_max, point.x);
            new_y_max = std::max(new_y_max, point.y);
        }

        cv::Rect2f transformed_bbox;
        transformed_bbox.x = new_x_min;
        transformed_bbox.y = new_y_min;
        transformed_bbox.width = new_x_max - new_x_min;
        transformed_bbox.height = new_y_max - new_y_min;

        return transformed_bbox;
    }

    std::vector<float> interp(const std::vector<float>& x, const std::vector<float>& xp, const std::vector<float>& fp) {
        std::vector<float> result;
        result.reserve(x.size());

        for (float xi : x) {
            if (xi <= xp.front()) {
                result.push_back(fp.front()); // 左边界处理
            } else if (xi >= xp.back()) {
                result.push_back(fp.back()); // 右边界处理
            } else {
                // 找到 xp 区间
                const auto upper = std::ranges::upper_bound(xp, xi);
                const int idx = static_cast<int>(std::distance(xp.begin(), upper) - 1);

                // 线性插值计算
                const float t = (xi - xp[idx]) / (xp[idx + 1] - xp[idx]);
                result.push_back(fp[idx] * (1 - t) + fp[idx + 1] * t);
            }
        }

        return result;
    }

} // namespace domain::face::helper
