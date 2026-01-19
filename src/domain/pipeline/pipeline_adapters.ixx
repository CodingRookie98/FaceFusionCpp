module;
#include <memory>
#include <map>
#include <string>
#include <any>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <iostream>

/**
 * @file pipeline_adapters.ixx
 * @brief Adapter classes to bridge Domain services to Pipeline FrameProcessors
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.pipeline:adapters;

import :api;
import :types;

import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.frame.enhancer;

import domain.face.helper;
import domain.face.masker;

namespace domain::pipeline {

/**
 * @brief Adapter for Face Swapper
 * @details Wraps IFaceSwapper to implement IFrameProcessor
 */
export class SwapperAdapter : public IFrameProcessor {
public:
    explicit SwapperAdapter(
        std::shared_ptr<face::swapper::IFaceSwapper> swapper,
        std::shared_ptr<face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_swapper(std::move(swapper)), m_occluder(std::move(occluder)),
        m_region_masker(std::move(region_masker)) {}

    void process(FrameData& frame) override {
        if (!m_swapper) return;

        if (frame.metadata.contains("swap_input")) {
            try {
                auto input =
                    std::any_cast<face::swapper::SwapInput>(frame.metadata.at("swap_input"));
                input.target_frame = frame.image;

                auto results = m_swapper->swap_face(input);

                for (const auto& res : results) {
                    // Compose Mask
                    face::masker::MaskCompositor::CompositionInput maskInput;
                    maskInput.size = res.crop_frame.size();
                    maskInput.options = res.mask_options;
                    maskInput.crop_frame =
                        res.target_crop_frame; // Use target crop for masking analysis
                    maskInput.occluder = m_occluder.get();
                    maskInput.region_masker = m_region_masker.get();

                    cv::Mat composedMask = face::masker::MaskCompositor::compose(maskInput);

                    // Paste back
                    frame.image = face::helper::paste_back(frame.image, res.crop_frame,
                                                           composedMask, res.affine_matrix);
                }
            } catch (const std::bad_any_cast& e) {
                std::cerr << "SwapperAdapter: Bad any cast for swap_input: " << e.what()
                          << std::endl;
            }
        }
    }

private:
    std::shared_ptr<face::swapper::IFaceSwapper> m_swapper;
    std::shared_ptr<face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<face::masker::IFaceRegionMasker> m_region_masker;
};

/**
 * @brief Adapter for Face Enhancer
 * @details Wraps IFaceEnhancer to implement IFrameProcessor
 */
export class FaceEnhancerAdapter : public IFrameProcessor {
public:
    explicit FaceEnhancerAdapter(
        std::shared_ptr<face::enhancer::IFaceEnhancer> enhancer,
        std::shared_ptr<face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_enhancer(std::move(enhancer)), m_occluder(std::move(occluder)),
        m_region_masker(std::move(region_masker)) {}

    void process(FrameData& frame) override {
        if (!m_enhancer) return;

        if (frame.metadata.contains("enhance_input")) {
            try {
                auto input =
                    std::any_cast<face::enhancer::EnhanceInput>(frame.metadata.at("enhance_input"));
                input.target_frame = frame.image;

                auto results = m_enhancer->enhance_face(input);

                if (results.empty()) return;

                cv::Mat working_frame = frame.image.clone();

                for (const auto& res : results) {
                    face::masker::MaskCompositor::CompositionInput maskInput;
                    maskInput.size = res.crop_frame.size();
                    maskInput.options = res.mask_options;
                    maskInput.crop_frame = res.target_crop_frame;
                    maskInput.occluder = m_occluder.get();
                    maskInput.region_masker = m_region_masker.get();

                    cv::Mat composedMask = face::masker::MaskCompositor::compose(maskInput);

                    working_frame = face::helper::paste_back(working_frame, res.crop_frame,
                                                             composedMask, res.affine_matrix);
                }

                // Global Face Blend
                // input.face_blend is 0-100
                if (input.face_blend >= 100) {
                    frame.image = working_frame;
                } else if (input.face_blend > 0) {
                    double alpha = input.face_blend / 100.0;
                    cv::addWeighted(working_frame, alpha, frame.image, 1.0 - alpha, 0.0,
                                    frame.image);
                }
                // if 0, do nothing (keep original frame)

            } catch (const std::bad_any_cast& e) {
                std::cerr << "FaceEnhancerAdapter: Bad any cast: " << e.what() << std::endl;
            }
        }
    }

private:
    std::shared_ptr<face::enhancer::IFaceEnhancer> m_enhancer;
    std::shared_ptr<face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<face::masker::IFaceRegionMasker> m_region_masker;
};

/**
 * @brief Adapter for Face Expression Restorer
 * @details Wraps IFaceExpressionRestorer to implement IFrameProcessor
 */
export class ExpressionAdapter : public IFrameProcessor {
public:
    explicit ExpressionAdapter(
        std::shared_ptr<face::expression::IFaceExpressionRestorer> restorer) :
        m_restorer(std::move(restorer)) {}

    void process(FrameData& frame) override {
        if (!m_restorer) return;

        if (frame.metadata.contains("expression_input")) {
            try {
                auto input = std::any_cast<face::expression::RestoreExpressionInput>(
                    frame.metadata.at("expression_input"));
                input.target_frame = frame.image;
                frame.image = m_restorer->restore_expression(input);
            } catch (const std::bad_any_cast& e) {
                std::cerr << "ExpressionAdapter: Bad any cast: " << e.what() << std::endl;
            }
        }
    }

private:
    std::shared_ptr<face::expression::IFaceExpressionRestorer> m_restorer;
};

/**
 * @brief Adapter for Frame Enhancer
 * @details Wraps IFrameEnhancer to implement IFrameProcessor
 */
export class FrameEnhancerAdapter : public IFrameProcessor {
public:
    explicit FrameEnhancerAdapter(std::shared_ptr<frame::enhancer::IFrameEnhancer> enhancer) :
        m_enhancer(std::move(enhancer)) {}

    void process(FrameData& frame) override {
        if (!m_enhancer) return;

        frame::enhancer::FrameEnhancerInput input;
        input.target_frame = frame.image; // cv::Mat shallow copy (ref counted)

        if (frame.metadata.contains("frame_enhancer_blend")) {
            try {
                input.blend =
                    std::any_cast<unsigned short>(frame.metadata.at("frame_enhancer_blend"));
            } catch (...) {}
        }

        frame.image = m_enhancer->enhance_frame(input);
    }

private:
    std::shared_ptr<frame::enhancer::IFrameEnhancer> m_enhancer;
};

} // namespace domain::pipeline
