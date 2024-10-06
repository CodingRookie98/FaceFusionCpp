/**
 ******************************************************************************
 * @file           : face_maskers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKERS_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKERS_H_

#include <shared_mutex>
#include "face_masker_base.h"
#include "face_masker_region.h"

class FaceMaskers {
public:
    FaceMaskers(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceMaskers();

    enum Type {
        Box,
        Occlusion,
        Region
    };

    static cv::Mat createStaticBoxMask(const cv::Size &cropSize, const float &faceMaskBlur,
                                       const std::array<int, 4> &faceMaskPadding);

    // Before using the function, please ensure that the function FaceMaskers::setFaceMaskPadding(const std::array<int, 4> &padding)
    // and FaceMaskers::setFaceMaskBlur(const float &faceMaskBlur) has been called at least once.
    cv::Mat createStaticBoxMask(const cv::Size &cropSize);

    cv::Mat createRegionMask(const cv::Mat &inputImage,
                             const std::unordered_set<FaceMaskerRegion::Region> &regions);

    // Before using the function, please ensure that the function FaceMaskers::setFaceMaskRegions(const std::unordered_set<FaceMaskerRegion> &regions) has been called at least once.
    cv::Mat createRegionMask(const cv::Mat &inputImage);

    cv::Mat createOcclusionMask(const cv::Mat &cropVisionFrame);
    void setFaceMaskPadding(const std::array<int, 4> &padding = {0, 0, 0, 0});
    void setFaceMaskBlur(const float &faceMaskBlur = 0.3);
    void setFaceMaskRegions(const std::unordered_set<FaceMaskerRegion::Region> &regions = {FaceMaskerRegion::All});
    static cv::Mat getBestMask(const std::vector<cv::Mat> &masks);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<Type, FaceMaskerBase *> m_maskers;
    std::shared_mutex m_mutex;
    std::array<int, 4> m_padding;
    float m_faceMaskBlur;
    std::unordered_set<FaceMaskerRegion::Region> m_regions;

    void createMasker(const Type &type);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKERS_H_
