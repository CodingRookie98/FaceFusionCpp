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

namespace ffc {

export enum class Gender { //  性别
    Male,
    Female,
};

export enum class Race { //  种族
    Black,               //  黑人
    Latino,              //  拉丁裔
    Indian,              //  印度裔
    Asian,               //  亚裔
    Arabic,              //  阿拉伯裔
    White,               //  白人
};


export struct AgeRange { //  年龄范围 [0, 100]
    unsigned short min{0};
    unsigned short max{100};

    // 判断年龄是否在范围内
    bool contains(unsigned short age) const {
        return age >= min && age <= max;
    }

    // 判断范围是否有效
    bool is_valid() const {
        return min <= max;
    }

    // 设置范围，并确保有效
    void set(const unsigned short& min_val, const unsigned short& max_val) {
        if (min_val <= max_val) {
            min = min_val;
            max = max_val;
        } else {
            min = max_val;
            max = min_val;
        }
    }
};

export class Face {
public:
    Face() = default;

    // Todo 将三个类型移到Face类之外
    typedef std::vector<float> Embedding;       //  特征向量
    typedef std::vector<cv::Point2f> Landmarks; // 5 个点或 68 个点
    typedef float Score;                        //  score 范围 [0, 1]

    cv::Rect2f m_box{};                    //  人脸框
    Landmarks m_landmark5{};         //  5 个点, 模型检测的输出结果
    Landmarks m_landmark68{};        //  68 个点, 模型检测的输出结果
    Landmarks m_landmark5_from_68{}; //  从 68 个点到 5 个点的映射，算法计算的输出结果
    Landmarks m_landmark68_from_5{}; //  从 5 个点到 68 个点的映射，模型检测的输出结果
    Embedding m_embedding{};         //  特征向量
    Embedding m_normed_embedding{};  //  归一化特征向量
    float m_detector_score{0};       //  检测器得分
    float m_landmarker_score{0};     //  关键点检测器得分
    Gender m_gender{Gender::Male};   //  性别
    AgeRange m_age_range{};          //  年龄范围
    Race m_race{Race::White};        //  种族

    [[nodiscard]] bool is_empty() const; //  是否为空
};

} // namespace ffc
