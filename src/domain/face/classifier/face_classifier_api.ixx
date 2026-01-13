module;
#include <opencv2/core/mat.hpp>

export module domain.face.classifier:api;

import domain.face; // for Race, Gender, AgeRange
import foundation.ai.inference_session;

export namespace domain::face::classifier {

    struct ClassificationResult {
        domain::face::Race race;
        domain::face::Gender gender;
        domain::face::AgeRange age;
    };

    class IFaceClassifier {
    public:
        virtual ~IFaceClassifier() = default;
        
        virtual void load_model(const std::string& model_path,
                                const foundation::ai::inference_session::Options& options = {}) = 0;

        virtual ClassificationResult classify(const cv::Mat& image, const domain::face::types::Landmarks& face_landmark_5) = 0;
    };
}
