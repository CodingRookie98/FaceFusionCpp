/**
 ******************************************************************************
 * @file           : face_selector.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_SELECTOR_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_SELECTOR_H_

#include "face.h"

class FaceSelector {
public:
    enum SelectorMode {
        Many,
        One,
        Reference
    };

    enum FaceSelectorOrder {
        Left_Right,
        Right_Left,
        Top_Bottom,
        Bottom_Top,
        Small_Large,
        Large_Small,
        Best_Worst,
        Worst_Best
    };

    enum Gender {
        Male,
        Female,
        AllGender,
    };

    enum Race {
        Black,
        Latino,
        Indian,
        Asian,
        Arabic,
        White,
        AllRace,
    };

    static std::vector<Face> sortByOrder(std::vector<Face> &faces, const FaceSelectorOrder &order);

    static std::vector<Face> filterByAge(std::vector<Face> faces, const unsigned int &ageStart, const unsigned int &ageEnd);

    static std::vector<Face> filterByGender(std::vector<Face> faces, const Gender &gender);

    static std::vector<Face> filterByRace(std::vector<Face> faces, const Race &race);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_SELECTOR_H_
