/**
 * @file common_types.ixx
 * @brief Common type definitions for the project
 * @author CodingRookie
 * @date 2026-01-18
 * @note Defines shared enum types and structures used across various domains.
 */
module;
#include <cstdint>

export module domain.common:types;

export namespace domain::common::types {

/**
 * @brief Gender classification
 */
enum class Gender : std::uint8_t {
    Male,   ///< Male gender
    Female, ///< Female gender
};

/**
 * @brief racial/ethnic classification
 * @note Used for technical classification purposes only.
 */
enum class Race : std::uint8_t {
    Black,  ///< Black/African descent
    Latino, ///< Latino/Hispanic descent
    Indian, ///< Indian/South Asian descent
    Asian,  ///< East Asian descent
    Arabic, ///< Middle Eastern/Arabic descent
    White,  ///< White/European descent
};

/**
 * @brief Represents an age range for classification results
 */
struct AgeRange {
    std::uint16_t min{0};   ///< Minimum age in the range
    std::uint16_t max{100}; ///< Maximum age in the range

    /**
     * @brief Check if a given age is within the range
     * @param age Age to check
     * @return true if age is in [min, max], false otherwise
     */
    [[nodiscard]] constexpr bool contains(std::uint16_t age) const noexcept {
        return age >= min && age <= max;
    }

    /**
     * @brief Check if the age range is logically valid (min <= max)
     * @return true if valid, false otherwise
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept { return min <= max; }

    /**
     * @brief Set the age range bounds
     * @param min_val Minimum age value
     * @param max_val Maximum age value
     * @note If min_val > max_val, the values will be swapped to ensure validity.
     */
    constexpr void set(std::uint16_t min_val, std::uint16_t max_val) noexcept {
        if (min_val <= max_val) {
            min = min_val;
            max = max_val;
        } else {
            min = max_val;
            max = min_val;
        }
    }
};

} // namespace domain::common::types
