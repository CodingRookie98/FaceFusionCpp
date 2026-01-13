/**
 ******************************************************************************
 * @file           : face_selector.cpp
 * @brief          : Face selector module implementation
 ******************************************************************************
 */

module;
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <opencv2/opencv.hpp> // For cv::Rect2f

module domain.face.selector;

import domain.face;
import domain.common;

namespace domain::face::selector {

// Internal helper functions
namespace {
std::vector<Face> sort_by_order(std::vector<Face> faces, const Order& order) {
    if (faces.empty()) { return faces; }

    switch (order) {
    case Order::LeftRight:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().x < face2.box().x;
        });
        break;
    case Order::RightLeft:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().x > face2.box().x;
        });
        break;
    case Order::TopBottom:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().y < face2.box().y;
        });
        break;
    case Order::BottomTop:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().y > face2.box().y;
        });
        break;
    case Order::SmallLarge:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().area() < face2.box().area();
        });
        break;
    case Order::LargeSmall:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.box().area() > face2.box().area();
        });
        break;
    case Order::BestWorst:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.detector_score() > face2.detector_score();
        });
        break;
    case Order::WorstBest:
        std::ranges::sort(faces, [](const Face& face1, const Face& face2) {
            return face1.detector_score() < face2.detector_score();
        });
        break;
    default: break;
    }

    return faces;
}

std::vector<Face> filter_by_race(std::vector<Face> faces,
                                 const std::unordered_set<domain::common::types::Race>& races) {
    if (faces.empty()) { return faces; }
    if (races.size() == ALL_RACES.size()) { return faces; }

    std::erase_if(faces, [&](const Face& face) { return !races.contains(face.race()); });
    return faces;
}

std::vector<Face> filter_by_gender(
    std::vector<Face> faces, const std::unordered_set<domain::common::types::Gender>& genders) {
    if (faces.empty()) { return faces; }
    if (genders.size() == ALL_GENDERS.size()) { return faces; }

    std::erase_if(faces, [&](const Face& face) { return !genders.contains(face.gender()); });
    return faces;
}

std::vector<Face> filter_by_age(std::vector<Face> faces, const unsigned int& age_start,
                                const unsigned int& age_end) {
    if (faces.empty()) { return faces; }

    std::erase_if(faces, [&](const Face& face) {
        const auto& range = face.age_range();
        return !(range.min >= age_start && range.max <= age_end);
    });
    return faces;
}
} // namespace

std::vector<Face> select_faces(const std::vector<Face>& faces, const Options& options) {
    auto res = filter_by_age(faces, options.age_start, options.age_end);
    res = filter_by_gender(res, options.genders);
    res = filter_by_race(res, options.races);
    res = sort_by_order(res, options.order);
    return res;
}

} // namespace domain::face::selector
