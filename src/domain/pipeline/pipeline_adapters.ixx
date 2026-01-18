module;
#include <memory>
#include <map>
#include <string>
#include <any>
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

namespace domain::pipeline {

/**
 * @brief Adapter for Face Swapper
 * @details Wraps IFaceSwapper to implement IFrameProcessor
 */
export class SwapperAdapter : public IFrameProcessor {
public:
    explicit SwapperAdapter(std::shared_ptr<face::swapper::IFaceSwapper> swapper) :
        m_swapper(std::move(swapper)) {}

    void process(FrameData& frame) override {
        if (!m_swapper) return;

        if (frame.metadata.contains("swap_input")) {
            try {
                auto input =
                    std::any_cast<face::swapper::SwapInput>(frame.metadata.at("swap_input"));
                input.target_frame = frame.image;

                frame.image = m_swapper->swap_face(input);
            } catch (const std::bad_any_cast& e) {
                std::cerr << "SwapperAdapter: Bad any cast for swap_input: " << e.what()
                          << std::endl;
            }
        }
    }

private:
    std::shared_ptr<face::swapper::IFaceSwapper> m_swapper;
};

/**
 * @brief Adapter for Face Enhancer
 * @details Wraps IFaceEnhancer to implement IFrameProcessor
 */
export class FaceEnhancerAdapter : public IFrameProcessor {
public:
    explicit FaceEnhancerAdapter(std::shared_ptr<face::enhancer::IFaceEnhancer> enhancer) :
        m_enhancer(std::move(enhancer)) {}

    void process(FrameData& frame) override {
        if (!m_enhancer) return;

        if (frame.metadata.contains("enhance_input")) {
            try {
                auto input =
                    std::any_cast<face::enhancer::EnhanceInput>(frame.metadata.at("enhance_input"));
                input.target_frame = frame.image;
                frame.image = m_enhancer->enhance_face(input);
            } catch (const std::bad_any_cast& e) {
                std::cerr << "FaceEnhancerAdapter: Bad any cast: " << e.what() << std::endl;
            }
        }
    }

private:
    std::shared_ptr<face::enhancer::IFaceEnhancer> m_enhancer;
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
