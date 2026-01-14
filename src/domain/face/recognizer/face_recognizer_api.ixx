module;
#include <vector>
#include <utility>
#include <opencv2/opencv.hpp>

export module domain.face.recognizer:api;

import domain.face;
import foundation.ai.inference_session;

namespace domain::face::recognizer {

/**
 * @brief Abstract base class for face recognizers
 */
export class FaceRecognizer : public foundation::ai::inference_session::InferenceSession {
public:
    virtual ~FaceRecognizer() = default;

    /**
     * @brief Extract face embedding from a face image
     * @param vision_frame Input image (full frame)
     * @param face_landmark_5 5-point face landmarks for alignment
     * @return Pair of {raw_embedding, normalized_embedding}
     */
    virtual std::pair<types::Embedding, types::Embedding> recognize(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) = 0;
};

} // namespace domain::face::recognizer
