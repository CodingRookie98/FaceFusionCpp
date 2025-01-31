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
#include <vector>

export module face_selector;
export import face;

namespace ffc {

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

    enum class Gender {
        Male = Face::Gender::Male,
        Female = Face::Gender::Female,
        AllGender = Male | Female,
    };

    enum class Race {
        Black = Face::Race::Black,
        Latino = Face::Race::Latino,
        Indian = Face::Race::Indian,
        Asian = Face::Race::Asian,
        Arabic = Face::Race::Arabic,
        White = Face::Race::White,
        AllRace = Black | Latino | Indian | Asian | Arabic | White,
    };

    struct Options {
        FaceSelectorOrder order = FaceSelectorOrder::Left_Right;
        Gender gender = Gender::AllGender;
        Race race = Race::AllRace;
        unsigned int ageStart = 0;
        unsigned int ageEnd = 100;
    };

    static std::vector<Face> select(const std::vector<Face> &faces, const Options &options);

private:
    static std::vector<Face> sortByOrder(std::vector<Face> faces, const FaceSelectorOrder &order);

    static std::vector<Face> filterByAge(std::vector<Face> faces, const unsigned int &ageStart, const unsigned int &ageEnd);

    static std::vector<Face> filterByGender(std::vector<Face> faces, const Gender &gender);

    static std::vector<Face> filterByRace(std::vector<Face> faces, const Race &race);
};

} // namespace ffc