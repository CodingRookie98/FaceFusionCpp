/**
 * @file face_classifier_api.ixx
 * @brief Face classifier interface definition
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core/mat.hpp>
#include <string>

export module domain.face.classifier:api;

import domain.face; // for Race, Gender, AgeRange
import foundation.ai.inference_session;

export namespace domain::face::classifier {

/**
 * @brief Result of face classification (gender, age, race)
 */
struct ClassificationResult {
    domain::face::Race race = domain::face::Race::White;     ///< Predicted race
    domain::face::Gender gender = domain::face::Gender::Male; ///< Predicted gender
    domain::face::AgeRange age;  ///< Predicted age range
};

/**
 * @brief Interface for Face Classifiers
 */
class IFaceClassifier {
public:
    virtual ~IFaceClassifier() = default;

    /**
     * @brief Load the classification model
     * @param model_path Path to the model file
     * @param options Inference session options
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options) = 0;

    /**
     * @brief Classify face attributes
     * @param image Input image containing the face
     * @param face_landmark_5 5-point landmarks for face alignment
     * @return ClassificationResult containing predicted attributes
     */
    virtual ClassificationResult classify(
        const cv::Mat& image, const domain::face::types::Landmarks& face_landmark_5) = 0;
};

} // namespace domain::face::classifier
