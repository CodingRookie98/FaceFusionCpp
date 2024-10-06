/**
 ******************************************************************************
 * @file           : face_masker_region.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_REGION_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_REGION_H_

#include "face_masker_base.h"

class FaceMaskerRegion : public FaceMaskerBase {
public:
    FaceMaskerRegion(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceMaskerRegion() override = default;

    enum Region {
        All = 0,
        Skin = 1,
        LeftEyebrow = 2,
        RightEyebrow = 3,
        LeftEye = 4,
        RightEye = 5,
        Glasses = 6,
        Nose = 10,
        Mouth = 11,
        UpperLip = 12,
        LowerLip = 13
    };
    
    cv::Mat createRegionMask(const cv::Mat &inputImage, const std::unordered_set<Region> &regions);

private:
    int m_inputHeight;
    int m_inputWidth;
    std::unordered_set<Region> m_allRegions = {
        Skin,
        LeftEyebrow,
        RightEyebrow,
        LeftEye,
        RightEye,
        Glasses,
        Nose,
        Mouth,
        UpperLip,
        LowerLip};
    
    std::vector<float> getInputImageData(const cv::Mat &image);
};



#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_MASKER_FACE_MASKER_REGION_H_
