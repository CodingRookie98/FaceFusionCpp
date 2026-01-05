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

std::vector<Face> FaceSelector::select(const std::vector<Face>& faces, const Options& options) {
    auto res = filterByAge(faces, options.age_start, options.age_end);
    res = filterByGender(res, options.genders);
    res = filterByRace(res, options.races);
    res = sortByOrder(res, options.order);
    return res;
}

std::vector<Face> FaceSelector::sortByOrder(std::vector<Face> faces, const FaceSelector::FaceSelectorOrder& order) {
    if (faces.empty()) {
        return faces;
    }

    switch (order) {
    case FaceSelectorOrder::Left_Right:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.x < face2.m_box.x;
        });
        break;
    case FaceSelectorOrder::Right_Left:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.x > face2.m_box.x;
        });
        break;
    case FaceSelectorOrder::Top_Bottom:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.y < face2.m_box.y;
        });
        break;
    case FaceSelectorOrder::Bottom_Top:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.y > face2.m_box.y;
        });
        break;
    case FaceSelectorOrder::Small_Large:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.area() < face2.m_box.area();
        });
        break;
    case FaceSelectorOrder::Large_Small:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_box.area() > face2.m_box.area();
        });
        break;
    case FaceSelectorOrder::Best_Worst:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_detector_score > face2.m_detector_score;
        });
        break;
    case FaceSelectorOrder::Worst_Best:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.m_detector_score < face2.m_detector_score;
        });
        break;
    default: break;
    }

    return faces;
}

std::vector<Face> FaceSelector::filterByRace(std::vector<Face> faces, const std::unordered_set<ffc::Race>& races) {
    if (faces.empty()) {
        return faces;
    }
    if (races.size() == std::unordered_set(utils::enum_all<ffc::Race>()).size()) {
        return faces;
    }
    // Erase if race is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (races.contains(static_cast<ffc::Race>(it->m_race))) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}

std::vector<Face> FaceSelector::filterByGender(std::vector<Face> faces, const std::unordered_set<ffc::Gender>& genders) {
    if (faces.empty()) {
        return faces;
    }
    if (genders.size() == std::unordered_set(utils::enum_all<ffc::Gender>()).size()) {
        return faces;
    }
    // Erase if gender is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (genders.contains(static_cast<ffc::Gender>(it->m_gender))) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}

std::vector<Face> FaceSelector::filterByAge(std::vector<Face> faces, const unsigned int& ageStart, const unsigned int& ageEnd) {
    if (faces.empty()) {
        return faces;
    }
    // Erase if age is not match (erase iterator is next iterator
    for (auto it = faces.begin(); it != faces.end();) {
        if (it->m_age_range.min >= ageStart && it->m_age_range.max <= ageEnd) {
            ++it;
        } else {
            it = faces.erase(it);
        }
    }
    return faces;
}
} // namespace ffc