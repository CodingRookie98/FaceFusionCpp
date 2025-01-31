/**
 ******************************************************************************
 * @file           : face_selector.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

module;
#include <vector>
#include <algorithm>

module face_selector;

namespace ffc {

std::vector<Face> FaceSelector::select(const std::vector<Face> &faces, const Options &options) {
    auto res = filterByAge(faces, options.ageStart, options.ageEnd);
    res = filterByGender(res, options.gender);
    res = filterByRace(res, options.race);
    res = sortByOrder(res, options.order);
    return res;
}

std::vector<Face> FaceSelector::sortByOrder(std::vector<Face> faces, const FaceSelector::FaceSelectorOrder &order) {
    if (faces.empty()) {
        return faces;
    }

    switch (order) {
    case FaceSelectorOrder::Left_Right:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_bBox.xMin < face2.m_bBox.xMin;
        });
        break;
    case FaceSelectorOrder::Right_Left:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_bBox.xMin > face2.m_bBox.xMin;
        });
        break;
    case FaceSelectorOrder::Top_Bottom:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_bBox.yMin < face2.m_bBox.yMin;
        });
        break;
    case FaceSelectorOrder::Bottom_Top:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_bBox.yMin > face2.m_bBox.yMin;
        });
        break;
    case FaceSelectorOrder::Small_Large:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return (face1.m_bBox.xMax - face1.m_bBox.xMin) * (face1.m_bBox.yMax - face1.m_bBox.yMin)
                   < (face2.m_bBox.xMax - face2.m_bBox.xMin) * (face2.m_bBox.yMax - face2.m_bBox.yMin);
        });
        break;
    case FaceSelectorOrder::Large_Small:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return (face1.m_bBox.xMax - face1.m_bBox.xMin) * (face1.m_bBox.yMax - face1.m_bBox.yMin)
                   > (face2.m_bBox.xMax - face2.m_bBox.xMin) * (face2.m_bBox.yMax - face2.m_bBox.yMin);
        });
        break;
    case FaceSelectorOrder::Best_Worst:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_detectorScore > face2.m_detectorScore;
        });
        break;
    case FaceSelectorOrder::Worst_Best:
        std::ranges::sort(faces, [](const Face &face1, const Face &face2) {
            return face1.m_detectorScore < face2.m_detectorScore;
        });
        break;
    default: break;
    }

    return faces;
}

std::vector<Face> FaceSelector::filterByRace(std::vector<Face> faces, const FaceSelector::Race &race) {
    if (faces.empty()) {
        return faces;
    }
    if (race == Race::AllRace) {
        return faces;
    }
    // Erase if race is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (static_cast<FaceSelector::Race>(it->m_race) == race) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}

std::vector<Face> FaceSelector::filterByGender(std::vector<Face> faces, const FaceSelector::Gender &gender) {
    if (faces.empty()) {
        return faces;
    }
    if (gender == Gender::AllGender) {
        return faces;
    }
    // Erase if gender is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (static_cast<FaceSelector::Gender>(it->m_gender) == gender) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}

std::vector<Face> FaceSelector::filterByAge(std::vector<Face> faces, const unsigned int &ageStart, const unsigned int &ageEnd) {
    if (faces.empty()) {
        return faces;
    }
    // Erase if age is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (it->m_age.min >= ageStart && it->m_age.max <= ageEnd) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}
} // namespace ffc