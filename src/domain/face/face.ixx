module;
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @file face.ixx
 * @brief Core Face entity definition
 * @author CodingRookie
 * @date 2026-01-27
 */
export module domain.face;

export import :types;
export import domain.common;

export namespace domain::face {
using domain::common::types::AgeRange;
using domain::common::types::Gender;
using domain::common::types::Race;

/**
 * @brief Face entity class
 * @details Represents a detected face with all its attributes (bbox, landmarks, embedding, etc.)
 */
class Face {
public:
    Face() = default;

    /** @name Accessors */
    ///@{

    /**
     * @brief Get the bounding box of the face
     * @return Const reference to the bounding box
     */
    [[nodiscard]] const cv::Rect2f& box() const noexcept;

    /**
     * @brief Get the face landmarks (keypoints)
     * @return Const reference to the landmarks
     */
    [[nodiscard]] const types::Landmarks& kps() const noexcept;

    /**
     * @brief Get the face embedding (feature vector)
     * @return Const reference to the embedding
     */
    [[nodiscard]] const types::Embedding& embedding() const noexcept;

    /**
     * @brief Get the normalized face embedding
     * @return Const reference to the normalized embedding
     */
    [[nodiscard]] const types::Embedding& normed_embedding() const noexcept;

    /**
     * @brief Get the gender of the face
     * @return Gender enum
     */
    [[nodiscard]] Gender gender() const noexcept;

    /**
     * @brief Get the age range of the face
     * @return Const reference to the age range
     */
    [[nodiscard]] const AgeRange& age_range() const noexcept;

    /**
     * @brief Get the race of the face
     * @return Race enum
     */
    [[nodiscard]] Race race() const noexcept;

    /**
     * @brief Get the detector confidence score
     * @return Score value
     */
    [[nodiscard]] types::Score detector_score() const noexcept;

    /**
     * @brief Get the landmarker confidence score
     * @return Score value
     */
    [[nodiscard]] types::Score landmarker_score() const noexcept;
    ///@}

    /** @name Auxiliary Accessors */
    ///@{
    /**
     * @brief Get 5-point landmarks
     * @details If current kps has 68 points, extracts 5 points; if 5 points, returns as is.
     * @return 5-point landmarks
     */
    [[nodiscard]] types::Landmarks get_landmark5() const;
    ///@}

    /** @name Modifiers */
    ///@{
    /**
     * @brief Set the bounding box
     * @param box New bounding box
     */
    void set_box(const cv::Rect2f& box) noexcept;

    /**
     * @brief Set the landmarks
     * @param kps New landmarks
     */
    void set_kps(types::Landmarks kps);

    /**
     * @brief Set the embedding
     * @param embedding New embedding
     */
    void set_embedding(types::Embedding embedding);

    /**
     * @brief Set the normalized embedding
     * @param embedding New normalized embedding
     */
    void set_normed_embedding(types::Embedding embedding);

    /**
     * @brief Set the gender
     * @param gender New gender
     */
    void set_gender(Gender gender) noexcept;

    /**
     * @brief Set the age range
     * @param age_range New age range
     */
    void set_age_range(const AgeRange& age_range) noexcept;

    /**
     * @brief Set the race
     * @param race New race
     */
    void set_race(Race race) noexcept;

    /**
     * @brief Set the detector confidence score
     * @param score New score
     */
    void set_detector_score(types::Score score) noexcept;

    /**
     * @brief Set the landmarker confidence score
     * @param score New score
     */
    void set_landmarker_score(types::Score score) noexcept;
    ///@}

    /** @name Business Logic */
    ///@{
    /**
     * @brief Check if the face is empty (invalid)
     * @return True if empty, false otherwise
     */
    [[nodiscard]] bool is_empty() const noexcept;
    ///@}

private:
    cv::Rect2f m_box{};
    types::Landmarks m_kps{};
    types::Embedding m_embedding{};
    types::Embedding m_normed_embedding{};
    types::Score m_detector_score{0.0f};
    types::Score m_landmarker_score{0.0f};
    Gender m_gender{Gender::Male};
    AgeRange m_age_range{};
    Race m_race{Race::White};
};
} // namespace domain::face
