export module domain.common:types;

export namespace domain::common::types {

enum class Gender {
    Male,
    Female,
};

enum class Race {
    Black,
    Latino,
    Indian,
    Asian,
    Arabic,
    White,
};

struct AgeRange {
    unsigned short min{0};
    unsigned short max{100};

    [[nodiscard]] constexpr bool contains(unsigned short age) const noexcept {
        return age >= min && age <= max;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept { return min <= max; }

    constexpr void set(unsigned short min_val, unsigned short max_val) noexcept {
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
