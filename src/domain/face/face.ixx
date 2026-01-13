module;
#include <opencv2/opencv.hpp>
#include <vector>

export module domain.face;

export import :types;
export import domain.common;

export namespace domain::face {
// 引入通用类型别名，方便使用
using domain::common::types::AgeRange;
using domain::common::types::Gender;
using domain::common::types::Race;

class Face {
public:
    Face() = default;

    // 访问器
    [[nodiscard]] const cv::Rect2f& box() const noexcept;
    [[nodiscard]] const types::Landmarks& kps() const noexcept; // 统一的关键点访问
    [[nodiscard]] const types::Embedding& embedding() const noexcept;
    [[nodiscard]] const types::Embedding& normed_embedding() const noexcept;
    [[nodiscard]] Gender gender() const noexcept;
    [[nodiscard]] const AgeRange& age_range() const noexcept;
    [[nodiscard]] Race race() const noexcept;
    [[nodiscard]] types::Score detector_score() const noexcept;
    [[nodiscard]] types::Score landmarker_score() const noexcept;

    // 辅助访问器 (按需计算)
    // 如果当前kps包含68点，提取5点；如果是5点直接返回
    [[nodiscard]] types::Landmarks get_landmark5() const;

    // 修改器
    void set_box(const cv::Rect2f& box) noexcept;
    void set_kps(types::Landmarks kps);
    void set_embedding(types::Embedding embedding);
    void set_normed_embedding(types::Embedding embedding);
    void set_gender(Gender gender) noexcept;
    void set_age_range(const AgeRange& age_range) noexcept;
    void set_race(Race race) noexcept;
    void set_detector_score(types::Score score) noexcept;
    void set_landmarker_score(types::Score score) noexcept;

    // 业务方法
    [[nodiscard]] bool is_empty() const noexcept;

private:
    cv::Rect2f m_box{};
    types::Landmarks m_kps{}; // 统一存储关键点
    types::Embedding m_embedding{};
    types::Embedding m_normed_embedding{};
    types::Score m_detector_score{0.0f};
    types::Score m_landmarker_score{0.0f};
    Gender m_gender{Gender::Male};
    AgeRange m_age_range{};
    Race m_race{Race::White};
};
} // namespace domain::face
