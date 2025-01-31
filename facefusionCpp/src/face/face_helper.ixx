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

export class FaceHelper {
public:
    FaceHelper() = default;
    ~FaceHelper() = default;

    enum class WarpTemplateType {
        Arcface_112_v1,
        Arcface_112_v2,
        Arcface_128_v2,
        Ffhq_512,
    };

    static std::vector<int>
    applyNms(const std::vector<Face::BBox> &boxes, std::vector<float> confidences, float nmsThresh);

    // return: 0->cropedVisionFrame, 1->affineMatrix
    static std::tuple<cv::Mat, cv::Mat> warpFaceByFaceLandmarks5(const cv::Mat &tempVisionFrame,
                                                                 const Face::Landmark &faceLandmark5,
                                                                 const std::vector<cv::Point2f> &warpTemplate,
                                                                 const cv::Size &cropSize);
    static std::tuple<cv::Mat, cv::Mat> warpFaceByFaceLandmarks5(const cv::Mat &tempVisionFrame,
                                                                 const Face::Landmark &faceLandmark5,
                                                                 const WarpTemplateType &warpTemplateType,
                                                                 const cv::Size &cropSize);

    static cv::Mat estimateMatrixByFaceLandmark5(const Face::Landmark &landmark5,
                                                 const std::vector<cv::Point2f> &warpTemplate,
                                                 const cv::Size &cropSize);

    static std::tuple<cv::Mat, cv::Mat> warpFaceByTranslation(const cv::Mat &tempVisionFrame,
                                                              const std::vector<float> &translation,
                                                              const float &scale,
                                                              const cv::Size &cropSize);

    static Face::Landmark
    convertFaceLandmark68To5(const Face::Landmark &faceLandmark68);

    static cv::Mat
    pasteBack(const cv::Mat &tempVisionFrame, const cv::Mat &cropVisionFrame,
              const cv::Mat &cropMask, const cv::Mat &affineMatrix);

    static std::vector<std::array<int, 2>>
    createStaticAnchors(const int &featureStride, const int &anchorTotal,
                        const int &strideHeight, const int &strideWidth);

    static Face::BBox
    distance2BBox(const std::array<int, 2> &anchor, const Face::BBox &bBox);

    static Face::Landmark
    distance2FaceLandmark5(const std::array<int, 2> &anchor, const Face::Landmark &faceLandmark5);

    static std::vector<cv::Point2f> getWarpTemplate(const WarpTemplateType &warpTemplateType);
    static std::vector<float> calcAverageEmbedding(const std::vector<std::vector<float>> &embeddings);
    static std::tuple<cv::Mat, cv::Size> createRotatedMatAndSize(const double &angle, const cv::Size &srcSize);
    static Face::BBox transformBBox(const Face::BBox &bBox, const cv::Mat &affineMatrix);
    static std::vector<cv::Point2f> transformPoints(const std::vector<cv::Point2f> &points,
                                                    const cv::Mat &affineMatrix);
    static std::vector<float> interp(const std::vector<float> &x, const std::vector<float> &xp,
                                     const std::vector<float> &fp);

private:
    static float getIoU(const Face::BBox &box1, const Face::BBox &box2);
};