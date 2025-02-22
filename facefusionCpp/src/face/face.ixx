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
    typedef std::vector<float> Embeddings;
    typedef std::vector<cv::Point2f> Landmarks;
    typedef float Score;
    struct Age {
        unsigned short min{0};
        unsigned short max{100};
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
    Landmarks m_landmark5;
    Landmarks m_landmark68;
    Landmarks m_landMark5From68;
    Landmarks m_landmark68From5;
    Embeddings m_embedding;
    Embeddings m_normedEmbedding;
    float m_detectorScore;
    float m_landmarkerScore;
    Gender m_gender;
    Age m_age;
    Race m_race;

    [[nodiscard]] bool isEmpty() const;
};
