/**
 * @file face_selector.ixx
 * @brief Utilities for filtering and sorting detected faces
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <optional>

export module domain.face.selector;

import domain.face;
import domain.common;

export namespace domain::face::selector {

/**
 * @brief High-level strategy for face selection
 */
enum class SelectorMode : std::uint8_t {
    Many,     ///< Process all detected faces
    One,      ///< Process only one face (based on sort order)
    Reference ///< Process faces matching a reference embedding
};

/**
 * @brief Sorting criteria for a list of faces
 */
enum class Order : std::uint8_t {
    LeftRight,  ///< Sort by X coordinate (ascending)
    RightLeft,  ///< Sort by X coordinate (descending)
    TopBottom,  ///< Sort by Y coordinate (ascending)
    BottomTop,  ///< Sort by Y coordinate (descending)
    SmallLarge, ///< Sort by bounding box area (ascending)
    LargeSmall, ///< Sort by bounding box area (descending)
    BestWorst,  ///< Sort by confidence score (descending)
    WorstBest   ///< Sort by confidence score (ascending)
};

/**
 * @brief Predefined set containing all genders
 */
const std::unordered_set<domain::common::types::Gender> kAllGenders = {
    domain::common::types::Gender::Male, domain::common::types::Gender::Female};

/**
 * @brief Predefined set containing all races
 */
const std::unordered_set<domain::common::types::Race> kAllRaces = {
    domain::common::types::Race::Black,  domain::common::types::Race::Latino,
    domain::common::types::Race::Indian, domain::common::types::Race::Asian,
    domain::common::types::Race::Arabic, domain::common::types::Race::White};

/**
 * @brief Configuration for face selection and filtering
 */
struct Options {
    SelectorMode mode = SelectorMode::Many;
    Order order = Order::LeftRight; ///< Primary sorting order
    std::unordered_set<domain::common::types::Gender> genders = kAllGenders; ///< Genders to keep
    std::unordered_set<domain::common::types::Race> races = kAllRaces;       ///< Races to keep
    unsigned int age_start = 0; ///< Minimum age (inclusive)
    unsigned int age_end = 100; ///< Maximum age (inclusive)

    // 相似度筛选参数
    std::optional<Face> reference_face;
    float similarity_threshold = 0.6F;
};

/**
 * @brief Filter and sort a list of faces based on criteria
 * @param faces Input vector of faces
 * @param options Selection and filtering configuration
 * @return Filtered and sorted vector of Face objects
 */
std::vector<Face> select_faces(const std::vector<Face>& faces, const Options& options);

} // namespace domain::face::selector
