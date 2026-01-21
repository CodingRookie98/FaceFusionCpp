/**
 * @file common_types.ixx
 * @brief 通用类型定义模块
 * @author CodingRookie
 * @date 2026-01-18
 * @note 定义项目中通用的枚举类型和结构体，用于人脸分析等业务场景
 */
export module domain.common:types;

export namespace domain::common::types {

/**
 * @brief 性别枚举
 * @details 用于人脸分析中的性别分类结果表示
 */
enum class Gender {
    Male,   ///< 男性
    Female, ///< 女性
};

/**
 * @brief 种族枚举
 * @details 用于人脸分析中的种族分类结果表示
 * @note 仅用于技术分类目的，不代表任何歧视性含义
 */
enum class Race {
    Black,  ///< 黑人
    Latino, ///< 拉丁裔
    Indian, ///< 印度裔
    Asian,  ///< 亚洲人
    Arabic, ///< 阿拉伯裔
    White,  ///< 白人
};

/**
 * @brief 年龄范围结构体
 * @details 表示一个年龄区间，用于人脸年龄估计的结果表示
 */
struct AgeRange {
    unsigned short min{0};   ///< 最小年龄
    unsigned short max{100}; ///< 最大年龄

    /**
     * @brief 检查指定年龄是否在范围内
     * @param age 要检查的年龄
     * @return 如果年龄在 [min, max] 范围内返回 true，否则返回 false
     */
    [[nodiscard]] constexpr bool contains(unsigned short age) const noexcept {
        return age >= min && age <= max;
    }

    /**
     * @brief 检查年龄范围是否有效
     * @return 如果 min <= max 返回 true，否则返回 false
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept { return min <= max; }

    /**
     * @brief 设置年龄范围
     * @param min_val 最小年龄值
     * @param max_val 最大年龄值
     * @note 如果 min_val > max_val，会自动交换两个值以确保范围有效
     */
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
