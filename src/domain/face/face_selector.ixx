/**
 ******************************************************************************
 * @file           : face_selector.ixx
 * @brief          : Face selector module interface
 ******************************************************************************
 */

module;
#include <vector>
#include <unordered_set>

export module domain.face.selector;

import domain.face;
import domain.common;

export namespace domain::face::selector {

    enum class SelectorMode {
        Many,
        One,
        Reference
    };

    enum class Order {
        LeftRight,
        RightLeft,
        TopBottom,
        BottomTop,
        SmallLarge,
        LargeSmall,
        BestWorst,
        WorstBest
    };

    // Helper to get all enum values
    const std::unordered_set<domain::common::types::Gender> ALL_GENDERS = {
        domain::common::types::Gender::Male,
        domain::common::types::Gender::Female
    };

    const std::unordered_set<domain::common::types::Race> ALL_RACES = {
        domain::common::types::Race::Black,
        domain::common::types::Race::Latino,
        domain::common::types::Race::Indian,
        domain::common::types::Race::Asian,
        domain::common::types::Race::Arabic,
        domain::common::types::Race::White
    };

    struct Options {
        Order order = Order::LeftRight;
        std::unordered_set<domain::common::types::Gender> genders = ALL_GENDERS;
        std::unordered_set<domain::common::types::Race> races = ALL_RACES;
        unsigned int age_start = 0;
        unsigned int age_end = 100;
    };

    std::vector<Face> select_faces(const std::vector<Face>& faces, const Options& options);

} // namespace domain::face::selector
