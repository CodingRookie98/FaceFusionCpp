/**
 ******************************************************************************
 * @file           : face_helper.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-4
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module face_helper;
export import face;

namespace ffc {

export namespace face_helper {

enum class WarpTemplateType {
    Arcface_112_v1,
    Arcface_112_v2,
    Arcface_128_v2,
    Ffhq_512,
};

std::vector<int>
applyNms(const std::vector<BBox>& boxes, std::vector<float> confidences, float nmsThresh);

// return: 0->cropedVisionFrame, 1->affineMatrix
std::tuple<cv::Mat, cv::Mat> warpFaceByFaceLandmarks5(const cv::Mat& tempVisionFrame,
                                                      const Face::Landmarks& faceLandmark5,
                                                      const std::vector<cv::Point2f>& warpTemplate,
                                                      const cv::Size& cropSize);
std::tuple<cv::Mat, cv::Mat> warpFaceByFaceLandmarks5(const cv::Mat& tempVisionFrame,
                                                      const Face::Landmarks& faceLandmark5,
                                                      const WarpTemplateType& warpTemplateType,
                                                      const cv::Size& cropSize);

cv::Mat estimateMatrixByFaceLandmark5(const Face::Landmarks& landmark5,
                                      const std::vector<cv::Point2f>& warpTemplate,
                                      const cv::Size& cropSize);

std::tuple<cv::Mat, cv::Mat> warpFaceByTranslation(const cv::Mat& tempVisionFrame,
                                                   const std::vector<float>& translation,
                                                   const float& scale,
                                                   const cv::Size& cropSize);

Face::Landmarks
convertFaceLandmark68To5(const Face::Landmarks& faceLandmark68);

cv::Mat
pasteBack(const cv::Mat& tempVisionFrame, const cv::Mat& cropVisionFrame,
          const cv::Mat& cropMask, const cv::Mat& affineMatrix);

std::vector<std::array<int, 2>>
createStaticAnchors(const int& featureStride, const int& anchorTotal,
                    const int& strideHeight, const int& strideWidth);

BBox distance2BBox(const std::array<int, 2>& anchor, const BBox& box);

Face::Landmarks
distance2FaceLandmark5(const std::array<int, 2>& anchor, const Face::Landmarks& faceLandmark5);

std::vector<cv::Point2f> getWarpTemplate(const WarpTemplateType& warpTemplateType);
std::vector<float> calcAverageEmbedding(const std::vector<std::vector<float>>& embeddings);
std::tuple<cv::Mat, cv::Size> createRotatedMatAndSize(const double& angle, const cv::Size& srcSize);
BBox transformBBox(const BBox& BoundingBox, const cv::Mat& affineMatrix);
std::vector<cv::Point2f> transformPoints(const std::vector<cv::Point2f>& points,
                                         const cv::Mat& affineMatrix);
std::vector<float> interp(const std::vector<float>& x, const std::vector<float>& xp,
                          const std::vector<float>& fp);
float getIoU(const BBox& box1, const BBox& box2);
}
} // namespace ffc::face_helper