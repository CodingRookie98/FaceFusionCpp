/**
 ******************************************************************************
 * @file           : face_recognizers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZERS_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZERS_H_

#include <shared_mutex>
#include "face_recognizer_base.h"

class FaceRecognizers {
public:
    explicit FaceRecognizers(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceRecognizers();

    enum FaceRecognizerType {
        Arc_w600k_r50,
    };

    // return: [0] embedding, [1] normedEmbedding
    std::array<Face::Embedding, 2>
    recognize(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5,
              const FaceRecognizerType &type);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::shared_mutex m_mutex;
    std::unordered_map<FaceRecognizerType, FaceRecognizerBase *> m_recognizers;

    void createRecognizer(const FaceRecognizerType &type);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZERS_H_
