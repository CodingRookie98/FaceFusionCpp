/**
 ******************************************************************************
 * @file           : face.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module face;

export class Face {
public:
    Face() = default;

    class BBox {
    public:
        float xMin{-1};
        float yMin{-1};
        float xMax{-1};
        float yMax{-1};

        [[nodiscard]] bool isEmpty() const {
            if (xMin == -1 || xMax == -1 || yMin == -1 || yMax == -1) {
                return true;
            }
            return false;
        }
    };
    typedef std::vector<float> Embedding;
    typedef std::vector<cv::Point2f> Landmark;
    typedef float Score;
    struct Age {
        ushort min{0};
        ushort max{100};
    };

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

    BBox m_bBox;
    Landmark m_landmark5;
    Landmark m_landmark68;
    Landmark m_landMark5By68;
    Landmark m_landmark68By5;
    Embedding m_embedding;
    Embedding m_normedEmbedding;
    float m_detectorScore;
    float m_landmarkerScore;
    Gender m_gender;
    Age m_age;
    Race m_race;

    [[nodiscard]] bool isEmpty() const;
};
