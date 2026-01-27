/**
 * @file face_analysis_processor.ixx
 * @brief Processor for high-level face analysis in the pipeline
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <utility>

export module services.pipeline.processors.face_analysis;

import domain.pipeline;
import domain.face.analyser;
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import foundation.infrastructure.logger;

namespace services::pipeline::processors {

using namespace domain::pipeline;

/**
 * @brief Configuration for required analysis data
 * @details Used to inform the processor which specific domain data (e.g. Swapper input)
 *          should be extracted and attached to frame metadata.
 */
export struct FaceAnalysisRequirements {
    bool need_swap_data = false;       ///< Prepare data for IFaceSwapper
    bool need_enhance_data = false;    ///< Prepare data for IFaceEnhancer
    bool need_expression_data = false; ///< Prepare data for IFaceExpressionRestorer
};

/**
 * @brief High-level pipeline processor for face detection and metadata preparation
 * @details Orchestrates FaceAnalyser to detect faces and populates FrameData metadata
 *          with inputs required by downstream processors.
 */
export class FaceAnalysisProcessor : public IFrameProcessor {
public:
    /**
     * @brief Construct a Face Analysis Processor
     * @param analyser Initialized FaceAnalyser instance
     * @param src_emb Source face embedding for reference (optional)
     * @param reqs Flags for required downstream data
     */
    FaceAnalysisProcessor(std::shared_ptr<domain::face::analyser::FaceAnalyser> analyser,
                          std::vector<float> src_emb, FaceAnalysisRequirements reqs) :
        m_analyser(std::move(analyser)), source_embedding(std::move(src_emb)), m_reqs(reqs) {}

    /**
     * @brief Detect faces and attach processing metadata to the frame
     */
    void process(FrameData& frame) override {
        auto faces = m_analyser->get_many_faces(
            frame.image, domain::face::analyser::FaceAnalysisType::Detection);

        if (faces.empty()) return;

        // 1. Prepare Swap Data
        if (m_reqs.need_swap_data) {
            domain::face::swapper::SwapInput swap_input;
            swap_input.target_frame = frame.image;
            for (const auto& face : faces) {
                swap_input.target_faces_landmarks.push_back(face.get_landmark5());
            }
            swap_input.source_embedding = source_embedding;
            frame.metadata["swap_input"] = std::move(swap_input);
        }

        // 2. Prepare Enhance Data
        if (m_reqs.need_enhance_data) {
            domain::face::enhancer::EnhanceInput enhance_input;
            enhance_input.target_frame = frame.image;
            for (const auto& face : faces) {
                enhance_input.target_faces_landmarks.push_back(face.get_landmark5());
            }
            enhance_input.face_blend = 80; // Default blend factor
            frame.metadata["enhance_input"] = std::move(enhance_input);
        }

        // 3. Prepare Expression Data
        if (m_reqs.need_expression_data) {
            domain::face::expression::RestoreExpressionInput expression_input;

            bool others_modify = m_reqs.need_swap_data || m_reqs.need_enhance_data;

            if (others_modify) {
                expression_input.source_frame = frame.image.clone();
            } else {
                expression_input.source_frame = frame.image;
            }

            for (const auto& face : faces) {
                expression_input.source_landmarks.push_back(face.get_landmark5());
            }
            frame.metadata["expression_input"] = std::move(expression_input);
        }
    }

private:
    std::shared_ptr<domain::face::analyser::FaceAnalyser> m_analyser;
    std::vector<float> source_embedding;
    FaceAnalysisRequirements m_reqs;
};

} // namespace services::pipeline::processors
