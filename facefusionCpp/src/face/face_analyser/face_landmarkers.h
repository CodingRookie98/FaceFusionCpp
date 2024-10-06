/**
 ******************************************************************************
 * @file           : face_landmarkers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_LANDMARKERS_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_LANDMARKERS_H_

#include <shared_mutex>
#include "face_landmarker_base.h"
#include "face.h"

class FaceLandmarkers {
public:
    explicit FaceLandmarkers(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceLandmarkers();

    enum class Landmarker68Model {
        Many,
        _2DFAN,
        PEPPA_WUTZ,
    };

    Face::Landmark expandLandmark68By5(const Face::Landmark &landmark5);
    std::tuple<Face::Landmark, float> detectLandmark68(const cv::Mat &image, const Face::BBox &Bbox, const Landmarker68Model &model);
    std::tuple<Face::Landmark, float> detectLandmark68(const cv::Mat &image, const Face::BBox &Bbox, const double &angle, const Landmarker68Model &model);

private:
    enum class LandmarkerModel {
        _2DFAN,
        _68By5,
        PEPPA_WUTZ,
    };
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<LandmarkerModel, FaceLandmarkerBase *> m_landmarkers;
    std::shared_mutex m_mutex;

    void createLandmarker(const LandmarkerModel &type);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_LANDMARKERS_H_
