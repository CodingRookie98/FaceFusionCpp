module;
#include <vector>
#include <opencv2/opencv.hpp>

module domain.face;

namespace domain::face {

    // 访问器实现
    const cv::Rect2f& Face::box() const noexcept { return m_box; }
    const types::Landmarks& Face::kps() const noexcept { return m_kps; }
    const types::Embedding& Face::embedding() const noexcept { return m_embedding; }
    const types::Embedding& Face::normed_embedding() const noexcept { return m_normed_embedding; }
    Gender Face::gender() const noexcept { return m_gender; }
    const AgeRange& Face::age_range() const noexcept { return m_age_range; }
    Race Face::race() const noexcept { return m_race; }
    types::Score Face::detector_score() const noexcept { return m_detector_score; }
    types::Score Face::landmarker_score() const noexcept { return m_landmarker_score; }

    // 辅助方法
    types::Landmarks Face::get_landmark5() const {
        if (m_kps.size() == 5) {
            return m_kps;
        }
        // TODO: 如果是68点，实现从68点提取5点的逻辑
        // 目前如果不是5点，返回空
        return {};
    }

    // 修改器实现
    void Face::set_box(const cv::Rect2f& box) noexcept { m_box = box; }
    void Face::set_kps(types::Landmarks kps) { m_kps = std::move(kps); }
    void Face::set_embedding(types::Embedding embedding) { m_embedding = std::move(embedding); }
    void Face::set_normed_embedding(types::Embedding embedding) { m_normed_embedding = std::move(embedding); }
    void Face::set_gender(Gender gender) noexcept { m_gender = gender; }
    void Face::set_age_range(const AgeRange& age_range) noexcept { m_age_range = age_range; }
    void Face::set_race(Race race) noexcept { m_race = race; }
    void Face::set_detector_score(types::Score score) noexcept { m_detector_score = score; }
    void Face::set_landmarker_score(types::Score score) noexcept { m_landmarker_score = score; }

    bool Face::is_empty() const noexcept {
        return m_box.area() <= 0.0f || m_kps.empty();
    }

}
