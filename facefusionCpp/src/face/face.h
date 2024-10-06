/**
 ******************************************************************************
 * @file           : face.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_H_

#include <vector>
#include <opencv2/opencv.hpp>

class Face {
public:
    Face() = default;

    class BBox {
    public:
        float xmin;
        float ymin;
        float xmax;
        float ymax;

        BBox(const float &xmin, const float &ymin, const float &xmax, const float &ymax) :
            xmin(xmin), ymin(ymin), xmax(xmax), ymax(ymax) {
        }
        BBox() {
            xmin = xmax = ymin = ymax = -1;
        }
        BBox(const BBox &bBox) {
            xmin = bBox.xmin;
            ymin = bBox.ymin;
            xmax = bBox.xmax;
            ymax = bBox.ymax;
        };
        bool isEmpty() const {
            if (xmin == -1 || xmax == -1 || ymin == -1 || ymax == -1) {
                return true;
            }
            return false;
        }
    };
    typedef std::vector<float> Embedding;
    typedef std::vector<cv::Point2f> Landmark;
    typedef float Score;
    struct Age {
        ushort min;
        ushort max;
    };
    enum Gender {
        Male,
        Female,
    };
    enum Race {
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
    Score m_detectorScore;
    Score m_landmarkerScore;
    Gender m_gender;
    Age m_age;
    Race m_race;

    bool isEmpty() const;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_H_
