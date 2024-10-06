/**
 ******************************************************************************
 * @file           : face_selector.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

#include "face_selector.h"

std::vector<Face> FaceSelector::sortByOrder(std::vector<Face> &faces, const FaceSelector::FaceSelectorOrder &order) {
    if (faces.empty()) {
        return faces;
    }

    switch (order) {
    case Left_Right:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return face1.m_bBox.xmin < face2.m_bBox.xmin;
        });
        break;
    case Right_Left:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return face1.m_bBox.xmin > face2.m_bBox.xmin;
        });
        break;
    case Top_Bottom:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return face1.m_bBox.ymin < face2.m_bBox.ymin;
        });
        break;
    case Bottom_Top:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return face1.m_bBox.ymin > face2.m_bBox.ymin;
        });
        break;
    case Small_Large:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return (face1.m_bBox.xmax - face1.m_bBox.xmin) * (face1.m_bBox.ymax - face1.m_bBox.ymin)
                   < (face2.m_bBox.xmax - face2.m_bBox.xmin) * (face2.m_bBox.ymax - face2.m_bBox.ymin);
        });
        break;
    case Large_Small:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return (face1.m_bBox.xmax - face1.m_bBox.xmin) * (face1.m_bBox.ymax - face1.m_bBox.ymin)
                   > (face2.m_bBox.xmax - face2.m_bBox.xmin) * (face2.m_bBox.ymax - face2.m_bBox.ymin);
        });
        break;
    case Best_Worst:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
            return face1.m_detectorScore > face2.m_detectorScore;
        });
        break;
    case Worst_Best:
        std::sort(faces.begin(), faces.end(), [](const Face &face1, const Face &face2) {
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
        FaceSelector::Race fs_race;
        switch (it->m_race) {
        case Face::Black: fs_race = Race::Black; break;
        case Face::Latino: fs_race = Race::Latino; break;
        case Face::Indian: fs_race = Race::Indian; break;
        case Face::Asian: fs_race = Race::Asian; break;
        case Face::Arabic: fs_race = Race::Arabic; break;
        case Face::White: fs_race = Race::White; break;
        }
        
        if (fs_race == race) {
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
        FaceSelector::Gender fs_gender;
        if (it->m_gender == Face::Male) {
            fs_gender = Gender::Male;
        } else {
            fs_gender = Gender::Female;
        }

        if (fs_gender == gender) {
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
