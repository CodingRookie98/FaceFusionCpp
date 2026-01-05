/**
 ******************************************************************************
 * @file           : face_selector.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-18
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <vector>
#include <google/protobuf/message.h>

export module face_selector;
export import face;
import utils;

namespace ffc {

using namespace infra;

export class FaceSelector {
public:
    enum class SelectorMode {
        Many,
        One,
        Reference
    };

    enum class FaceSelectorOrder {
        Left_Right,
        Right_Left,
        Top_Bottom,
        Bottom_Top,
        Small_Large,
        Large_Small,
        Best_Worst,
        Worst_Best
    };

    struct Options {
        FaceSelectorOrder order = FaceSelectorOrder::Left_Right;
        std::unordered_set<ffc::Gender> genders{utils::enum_all<ffc::Gender>()};
        std::unordered_set<ffc::Race> races{utils::enum_all<ffc::Race>()};
        unsigned int age_start = 0;
        unsigned int age_end = 100;
    };

    static std::vector<Face> select(const std::vector<Face>& faces, const Options& options);

private:
    static std::vector<Face> sortByOrder(std::vector<Face> faces, const FaceSelectorOrder& order);

    static std::vector<Face> filterByAge(std::vector<Face> faces, const unsigned int& ageStart, const unsigned int& ageEnd);

    static std::vector<Face> filterByGender(std::vector<Face> faces, const std::unordered_set<ffc::Gender>& genders);

    static std::vector<Face> filterByRace(std::vector<Face> faces, const std::unordered_set<ffc::Race>& races);
};

} // namespace ffc