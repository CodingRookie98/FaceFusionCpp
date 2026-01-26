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
 * @brief Analysis Requirements
 *
 * Flags to indicate which downstream data should be prepared.
 */
export struct FaceAnalysisRequirements {
    bool need_swap_data = false;
    bool need_enhance_data = false;
    bool need_expression_data = false;
};

/**
 * @brief Face Analysis Processor
 *
 * Performs face detection and prepares context data for downstream processors
 * (Swapper, Enhancer, Expression Restorer).
 */
export class FaceAnalysisProcessor : public IFrameProcessor {
public:
    FaceAnalysisProcessor(std::shared_ptr<domain::face::analyser::FaceAnalyser> analyser,
                          std::vector<float> src_emb, FaceAnalysisRequirements reqs) :
        m_analyser(std::move(analyser)), source_embedding(std::move(src_emb)), m_reqs(reqs) {}

    void process(FrameData& frame) override {
        // Use Detection only (implicitly includes 5 landmarks)
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
            // Note: face_blend is enhancer-specific param, should not be hardcoded here strictly,
            // but currently the enhancer adapter expects it in input struct.
            // Ideally, this should be part of Enhancer's own config, not metadata.
            // For now, we keep compatibility but might consider moving 'face_blend' handling to
            // EnhancerAdapter.
            enhance_input.face_blend = 80; // Legacy default
            frame.metadata["enhance_input"] = std::move(enhance_input);
        }

        // 3. Prepare Expression Data
        if (m_reqs.need_expression_data) {
            domain::face::expression::RestoreExpressionInput expression_input;

            // Optimization: Only clone if other processors will modify the frame.
            // If Swapper or Enhancer are active, they might modify the frame in-place or return a
            // new one, losing the original appearance which Expression Restorer needs as reference.
            bool others_modify = m_reqs.need_swap_data || m_reqs.need_enhance_data;

            if (others_modify) {
                expression_input.source_frame = frame.image.clone();
            } else {
                // If only expression restoration is active, we can safely share the reference.
                // Note: ExpressionAdapter uses target_frame = frame.image, and restore_expression
                // returns a new image.
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