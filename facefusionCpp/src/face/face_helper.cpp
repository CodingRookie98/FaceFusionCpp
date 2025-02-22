/**
 ******************************************************************************
 * @file           : face_helper.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-4
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

module face_helper;
import face;

float face_helper::getIoU(const Face::BBox &box1, const Face::BBox &box2) {
    const float x1 = std::max(box1.xMin, box2.xMin);
    const float y1 = std::max(box1.yMin, box2.yMin);
    const float x2 = std::min(box1.xMax, box2.xMax);
    const float y2 = std::min(box1.yMax, box2.yMax);
    const float w = std::max(0.f, x2 - x1);
    const float h = std::max(0.f, y2 - y1);
    const float overArea = w * h;
    if (overArea == 0)
        return 0.0;
    const float unionArea = (box1.xMax - box1.xMin) * (box1.yMax - box1.yMin) + (box2.xMax - box2.xMin) * (box2.yMax - box2.yMin) - overArea;
    return overArea / unionArea;
}

std::vector<int> face_helper::applyNms(const std::vector<Face::BBox> &boxes,
                                      std::vector<float> confidences,
                                      const float nmsThresh) {
    std::ranges::sort(confidences, [&confidences](const size_t index1, const size_t index2) {
        return confidences[index1] > confidences[index2];
    });
    const size_t numBox = confidences.size();
    std::vector<bool> isSuppressed(numBox, false);
    for (size_t i = 0; i < numBox; ++i) {
        if (isSuppressed[i]) {
            continue;
        }
        for (size_t j = i + 1; j < numBox; ++j) {
            if (isSuppressed[j]) {
                continue;
            }

            if (const float ovr = getIoU(boxes[i], boxes[j]); ovr > nmsThresh) {
                isSuppressed[j] = true;
            }
        }
    }

    std::vector<int> keepInds;
    for (int i = 0; i < isSuppressed.size(); i++) {
        if (!isSuppressed[i]) {
            keepInds.emplace_back(i);
        }
    }
    return keepInds;
}

std::tuple<cv::Mat, cv::Mat>
face_helper::warpFaceByFaceLandmarks5(const cv::Mat &tempVisionFrame,
                                     const Face::Landmarks &faceLandmark5,
                                     const std::vector<cv::Point2f> &warpTemplate,
                                     const cv::Size &cropSize) {
    cv::Mat affineMatrix = estimateMatrixByFaceLandmark5(faceLandmark5, warpTemplate, cropSize);
    cv::Mat cropVision;
    cv::warpAffine(tempVisionFrame, cropVision, affineMatrix, cropSize, cv::INTER_AREA, cv::BORDER_REPLICATE);
    return std::make_tuple(cropVision, affineMatrix);
}

std::tuple<cv::Mat, cv::Mat> face_helper::warpFaceByFaceLandmarks5(const cv::Mat &tempVisionFrame, const Face::Landmarks &faceLandmark5, const face_helper::WarpTemplateType &warpTemplateType, const cv::Size &cropSize) {
    std::vector<cv::Point2f> warpTemplate = getWarpTemplate(warpTemplateType);
    return warpFaceByFaceLandmarks5(tempVisionFrame, faceLandmark5, warpTemplate, cropSize);
}

cv::Mat face_helper::estimateMatrixByFaceLandmark5(const Face::Landmarks &landmark5,
                                                  const std::vector<cv::Point2f> &warpTemplate,
                                                  const cv::Size &cropSize) {
    std::vector<cv::Point2f> normedWarpTemplate = warpTemplate;
    for (auto &point : normedWarpTemplate) {
        point.x *= static_cast<float>(cropSize.width);
        point.y *= static_cast<float>(cropSize.height);
    }
    cv::Mat affineMatrix = cv::estimateAffinePartial2D(landmark5, normedWarpTemplate,
                                                       cv::noArray(), cv::RANSAC, 100);
    return affineMatrix;
}

std::tuple<cv::Mat, cv::Mat>
face_helper::warpFaceByTranslation(const cv::Mat &tempVisionFrame,
                                  const std::vector<float> &translation,
                                  const float &scale, const cv::Size &cropSize) {
    cv::Mat affineMatrix = (cv::Mat_<float>(2, 3) << scale, 0.f, translation[0], 0.f, scale, translation[1]);
    cv::Mat cropImg;
    warpAffine(tempVisionFrame, cropImg, affineMatrix, cropSize);
    return std::make_tuple(cropImg, affineMatrix);
}

Face::Landmarks
face_helper::convertFaceLandmark68To5(const Face::Landmarks &faceLandmark68) {
    Face::Landmarks faceLandmark5_68(5);
    faceLandmark5_68.resize(5);
    float x = 0, y = 0;
    for (int i = 36; i < 42; i++) { /// left_eye
        x += faceLandmark68[i].x;
        y += faceLandmark68[i].y;
    }
    x /= 6;
    y /= 6;
    faceLandmark5_68[0] = cv::Point2f(x, y); /// left_eye

    x = 0, y = 0;
    for (int i = 42; i < 48; i++) { /// right_eye

        x += faceLandmark68[i].x;
        y += faceLandmark68[i].y;
    }
    x /= 6;
    y /= 6;
    faceLandmark5_68[1] = cv::Point2f(x, y);  /// right_eye
    faceLandmark5_68[2] = faceLandmark68[30]; /// nose
    faceLandmark5_68[3] = faceLandmark68[48]; /// left_mouth_end
    faceLandmark5_68[4] = faceLandmark68[54]; /// right_mouth_end

    return faceLandmark5_68;
}

cv::Mat face_helper::pasteBack(const cv::Mat &tempVisionFrame, const cv::Mat &cropVisionFrame,
                              const cv::Mat &cropMask, const cv::Mat &affineMatrix) {
    cv::Mat inverseMatrix;
    cv::invertAffineTransform(affineMatrix, inverseMatrix);
    cv::Mat inverseMask;
    const cv::Size tempSize(tempVisionFrame.cols, tempVisionFrame.rows);
    warpAffine(cropMask, inverseMask, inverseMatrix, tempSize);
    inverseMask.setTo(0, inverseMask < 0);
    inverseMask.setTo(1, inverseMask > 1);
    cv::Mat inverseVisionFrame;
    warpAffine(cropVisionFrame, inverseVisionFrame, inverseMatrix, tempSize, cv::INTER_LINEAR, cv::BORDER_REPLICATE);

    std::vector<cv::Mat> inverseVisionFrameBgrs(3);
    split(inverseVisionFrame, inverseVisionFrameBgrs);
    std::vector<cv::Mat> tempVisionFrameBgrs(3);
    split(tempVisionFrame, tempVisionFrameBgrs);
    for (int c = 0; c < 3; c++) {
        inverseVisionFrameBgrs[c].convertTo(inverseVisionFrameBgrs[c], CV_32FC1); ////注意数据类型转换，不然在下面的矩阵点乘运算时会报错的
        tempVisionFrameBgrs[c].convertTo(tempVisionFrameBgrs[c], CV_32FC1);       ////注意数据类型转换，不然在下面的矩阵点乘运算时会报错的
    }
    std::vector<cv::Mat> channelMats(3);

    channelMats[0] = inverseMask.mul(inverseVisionFrameBgrs[0]) + tempVisionFrameBgrs[0].mul(1 - inverseMask);
    channelMats[1] = inverseMask.mul(inverseVisionFrameBgrs[1]) + tempVisionFrameBgrs[1].mul(1 - inverseMask);
    channelMats[2] = inverseMask.mul(inverseVisionFrameBgrs[2]) + tempVisionFrameBgrs[2].mul(1 - inverseMask);

    cv::Mat pasteVisionFrame;
    merge(channelMats, pasteVisionFrame);
    pasteVisionFrame.convertTo(pasteVisionFrame, CV_8UC3);
    return pasteVisionFrame;
}

std::vector<std::array<int, 2>>
face_helper::createStaticAnchors(const int &featureStride, const int &anchorTotal,
                                const int &strideHeight, const int &strideWidth) {
    std::vector<std::array<int, 2>> anchors;

    // Create a grid of (y, x) coordinates
    for (int i = 0; i < strideHeight; ++i) {
        for (int j = 0; j < strideWidth; ++j) {
            // Compute the original image coordinates
            const int y = i * featureStride;
            const int x = j * featureStride;

            // Add each anchor for the current grid point
            for (int k = 0; k < anchorTotal; ++k) {
                anchors.push_back({y, x});
            }
        }
    }

    return anchors;
}

Face::BBox face_helper::distance2BBox(const std::array<int, 2> &anchor, const Face::BBox &bBox) {
    Face::BBox result;
    result.xMin = static_cast<float>(anchor[1]) - bBox.xMin;
    result.yMin = static_cast<float>(anchor[0]) - bBox.yMin;
    result.xMax = static_cast<float>(anchor[1]) + bBox.xMax;
    result.yMax = static_cast<float>(anchor[0]) + bBox.yMax;
    return result;
}

Face::Landmarks
face_helper::distance2FaceLandmark5(const std::array<int, 2> &anchor, const Face::Landmarks &faceLandmark5) {
    Face::Landmarks faceLandmark5_;
    faceLandmark5_.resize(5);
    for (int i = 0; i < 5; ++i) {
        faceLandmark5_[i].x = faceLandmark5[i].x + static_cast<float>(anchor[1]);
        faceLandmark5_[i].y = faceLandmark5[i].y + static_cast<float>(anchor[0]);
    }
    return faceLandmark5_;
}

std::vector<cv::Point2f> face_helper::getWarpTemplate(const WarpTemplateType &warpTemplateType) {
    static std::unique_ptr<std::map<WarpTemplateType, std::vector<cv::Point2f>>> warpTemplates = nullptr;
    if (warpTemplates == nullptr) {
        warpTemplates = std::make_unique<std::map<WarpTemplateType, std::vector<cv::Point2f>>>(std::map<WarpTemplateType, std::vector<cv::Point2f>>(
            {{WarpTemplateType::Arcface_112_v1, {{0.35473214, 0.45658929}, {0.64526786, 0.45658929}, {0.50000000, 0.61154464}, {0.37913393, 0.77687500}, {0.62086607, 0.77687500}}},
             {WarpTemplateType::Arcface_112_v2, {{0.34191607, 0.46157411}, {0.65653393, 0.45983393}, {0.50022500, 0.64050536}, {0.37097589, 0.82469196}, {0.63151696, 0.82325089}}},
             {WarpTemplateType::Arcface_128_v2, {{0.36167656, 0.40387734}, {0.63696719, 0.40235469}, {0.50019687, 0.56044219}, {0.38710391, 0.72160547}, {0.61507734, 0.72034453}}},
             {WarpTemplateType::Ffhq_512, {{0.37691676, 0.46864664}, {0.62285697, 0.46912813}, {0.50123859, 0.61331904}, {0.39308822, 0.72541100}, {0.61150205, 0.72490465}}}}));
    }
    return warpTemplates->at(warpTemplateType);
}

std::vector<float> face_helper::calcAverageEmbedding(const std::vector<std::vector<float>> &embeddings) {
    std::vector<float> averageEmbedding(embeddings[0].size(), 0.0);
    for (const auto &embedding : embeddings) {
        for (size_t i = 0; i < embedding.size(); ++i) {
            averageEmbedding[i] += embedding[i];
        }
    }
    for (float &value : averageEmbedding) {
        value /= static_cast<float>(embeddings.size());
    }
    return averageEmbedding;
}

std::tuple<cv::Mat, cv::Size> face_helper::createRotatedMatAndSize(const double &angle, const cv::Size &srcSize) {
    cv::Mat rotatedMat = cv::getRotationMatrix2D(cv::Point2f(static_cast<float>(srcSize.width) / 2.f, static_cast<float>(srcSize.height) / 2.f), angle, 1.0);
    const cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), srcSize, static_cast<float>(angle)).boundingRect2f();
    rotatedMat.at<double>(0, 2) += (bbox.width - static_cast<float>(srcSize.width)) * 0.5;
    rotatedMat.at<double>(1, 2) += (bbox.height - static_cast<float>(srcSize.height)) * 0.5;
    cv::Size rotatedSize(static_cast<int>(bbox.width), static_cast<int>(bbox.height));
    return std::make_tuple(rotatedMat, rotatedSize);
}

std::vector<cv::Point2f> face_helper::transformPoints(const std::vector<cv::Point2f> &points, const cv::Mat &affineMatrix) {
    std::vector<cv::Point2f> transformedPoints;
    cv::transform(points, transformedPoints, affineMatrix);
    return transformedPoints;
}

Face::BBox face_helper::transformBBox(const Face::BBox &bBox, const cv::Mat &affineMatrix) {
    const std::vector<cv::Point2f> points = {{bBox.xMin, bBox.yMin}, {bBox.xMax, bBox.yMax}};
    const std::vector<cv::Point2f> transformedPoints = transformPoints(points, affineMatrix);
    const float newXMin = std::min(transformedPoints[0].x, transformedPoints[1].x);
    const float newYMin = std::min(transformedPoints[0].y, transformedPoints[1].y);
    const float newXMax = std::max(transformedPoints[0].x, transformedPoints[1].x);
    const float newYMax = std::max(transformedPoints[0].y, transformedPoints[1].y);
    const Face::BBox transformedBBox{newXMin, newYMin, newXMax, newYMax};
    return transformedBBox;
}

std::vector<float> face_helper::interp(const std::vector<float> &x, const std::vector<float> &xp, const std::vector<float> &fp) {
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
