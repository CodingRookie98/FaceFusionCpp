module;
#include <memory>
#include <map>
#include <string>
#include <any>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <iostream>
#include <mutex>
#include <format>

/**
 * @file pipeline_adapters.ixx
 * @brief Adapter classes to bridge Domain services to Pipeline FrameProcessors
 * @author CodingRookie
 * @date 2026-01-27
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
import foundation.ai.inference_session;
import foundation.infrastructure.logger;

export namespace domain::pipeline {

// Forward declaration
void RegisterBuiltinAdapters();

/**
 * @brief Adapter for Face Swapper
 * @details Wraps IFaceSwapper to implement IFrameProcessor
 * interface for the pipeline. Handles model loading, masking (occlusion and region), and pasting
 * the result back.
 */
export class SwapperAdapter : public IFrameProcessor {
public:
    /**
     * @brief Construct a new Swapper Adapter
     * @param context_ptr Pointer to
     * PipelineContext (must be casted internally)
     */
    static std::shared_ptr<IFrameProcessor> create(const void* context_ptr);

private:
    /**
     * @brief Internal constructor used by create()
     */
    explicit SwapperAdapter(
        std::shared_ptr<face::swapper::IFaceSwapper> swapper, std::string model_path,
        foundation::ai::inference_session::Options options,
        std::shared_ptr<face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_swapper(std::move(swapper)), m_model_path(std::move(model_path)),
        m_options(std::move(options)), m_occluder(std::move(occluder)),
        m_region_masker(std::move(region_masker)) {}

public:
    ~SwapperAdapter() override {
        // Explicit destructor helps with stability across module boundaries
    }

    /**
     * @brief Ensures the swapper model is loaded into memory
     */
    void ensure_loaded() override {
        if (m_loaded) return;
        std::lock_guard<std::mutex> lock(m_load_mutex);
        if (m_loaded) return;

        if (m_swapper && !m_model_path.empty()) {
            m_swapper->load_model(m_model_path, m_options);
            m_input_size = m_swapper->get_model_input_size();

            // Dynamic template selection based on model input size
            if (m_input_size.width == 128) {
                m_template_type = domain::face::helper::WarpTemplateType::Arcface_128_v2;
            } else if (m_input_size.width == 512) {
                m_template_type = domain::face::helper::WarpTemplateType::Ffhq_512;
            } else {
                // Default fallback
                m_template_type = domain::face::helper::WarpTemplateType::Arcface_128_v2;
            }
        }
        m_loaded = true;
    }

    /**
     * @brief Process a single frame using the face swapper
     * @param frame The frame
     * data to be processed and updated
     */
    void process(FrameData& frame) override {
        foundation::infrastructure::logger::ScopedTimer timer("SwapperAdapter::process");
        ensure_loaded();
        if (!m_swapper) return;

        if (frame.swap_input.has_value()) {
            try {
                auto& input = frame.swap_input.value();
                if (input.target_faces_landmarks.empty()) return;

                cv::Mat working_frame = frame.image;

                for (const auto& landmarks : input.target_faces_landmarks) {
                    // 1. Warp / Crop
                    auto [crop_frame, affine_matrix] = face::helper::warp_face_by_face_landmarks_5(
                        working_frame, landmarks, m_template_type, m_input_size);

                    // 2. Inference
                    cv::Mat swapped_crop = m_swapper->swap_face(crop_frame, input.source_embedding);

                    // 3. Color Match (Task 3.3)
                    cv::Mat matched_crop =
                        face::helper::apply_color_match(crop_frame, swapped_crop);

                    // 4. Compose Mask
                    face::masker::MaskCompositor::CompositionInput maskInput;
                    maskInput.size = m_input_size;
                    maskInput.options = input.mask_options;
                    maskInput.crop_frame = crop_frame;
                    maskInput.occluder = m_occluder.get();
                    maskInput.region_masker = m_region_masker.get();

                    cv::Mat composedMask = face::masker::MaskCompositor::compose(maskInput);

                    // 5. Paste back
                    working_frame = face::helper::paste_back(working_frame, matched_crop,
                                                             composedMask, affine_matrix);
                }
                frame.image = working_frame;

            } catch (const std::exception& e) {
                foundation::infrastructure::logger::Logger::get_instance()->error(
                    std::format("[E701] SwapperAdapter::process failed: {}", e.what()));
            }
        }
    }

private:
    std::shared_ptr<face::swapper::IFaceSwapper> m_swapper;
    std::string m_model_path;
    foundation::ai::inference_session::Options m_options;
    bool m_loaded = false;
    std::mutex m_load_mutex;

    std::shared_ptr<face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<face::masker::IFaceRegionMasker> m_region_masker;

    cv::Size m_input_size{128, 128}; // Default, updated on load
    domain::face::helper::WarpTemplateType m_template_type =
        domain::face::helper::WarpTemplateType::Arcface_128_v2;
};

/**
 * @brief Adapter for Face Enhancer
 * @details Wraps IFaceEnhancer to implement IFrameProcessor interface for the pipeline.
 *          Handles face enhancement, global face blending, and pasting back.
 */
export class FaceEnhancerAdapter : public IFrameProcessor {
public:
    /**
     * @brief Construct a new Face Enhancer Adapter
     * @param context_ptr Pointer to
     * PipelineContext
     */
    static std::shared_ptr<IFrameProcessor> create(const void* context_ptr);

private:
    /**
     * @brief Internal constructor
     */
    explicit FaceEnhancerAdapter(
        std::shared_ptr<face::enhancer::IFaceEnhancer> enhancer, std::string model_path,
        foundation::ai::inference_session::Options options,
        std::shared_ptr<face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_enhancer(std::move(enhancer)), m_model_path(std::move(model_path)),
        m_options(std::move(options)), m_occluder(std::move(occluder)),
        m_region_masker(std::move(region_masker)) {}

public:
    /**
     * @brief Ensures the enhancer model is loaded into memory
     */
    void ensure_loaded() override {
        if (m_loaded) return;
        std::lock_guard<std::mutex> lock(m_load_mutex);
        if (m_loaded) return;

        if (m_enhancer && !m_model_path.empty()) {
            m_enhancer->load_model(m_model_path, m_options);
            m_input_size = m_enhancer->get_model_input_size();

            // Dynamic template selection based on model input size
            if (m_input_size.width == 128) {
                m_template_type = domain::face::helper::WarpTemplateType::Arcface_128_v2;
            } else if (m_input_size.width == 512) {
                m_template_type = domain::face::helper::WarpTemplateType::Ffhq_512;
            } else {
                // Default fallback
                m_template_type = domain::face::helper::WarpTemplateType::Ffhq_512;
            }
        }
        m_loaded = true;
    }

    /**
     * @brief Process a single frame using the face enhancer
     * @param frame The frame data to be processed and updated
     */
    void process(FrameData& frame) override {
        foundation::infrastructure::logger::ScopedTimer timer("FaceEnhancerAdapter::process");
        ensure_loaded();
        if (!m_enhancer) return;

        if (frame.enhance_input.has_value()) {
            try {
                auto& input = frame.enhance_input.value();
                if (input.target_faces_landmarks.empty()) return;

                cv::Mat working_frame = frame.image.clone();

                for (const auto& landmarks : input.target_faces_landmarks) {
                    // 1. Warp
                    auto [crop_frame, affine_matrix] = face::helper::warp_face_by_face_landmarks_5(
                        frame.image, landmarks, m_template_type, m_input_size);

                    // 2. Inference
                    cv::Mat enhanced_crop = m_enhancer->enhance_face(crop_frame);

                    if (enhanced_crop.empty()) continue;

                    // 3. Compose Mask
                    face::masker::MaskCompositor::CompositionInput maskInput;
                    maskInput.size = m_input_size;
                    maskInput.options = input.mask_options;
                    maskInput.crop_frame = crop_frame;
                    maskInput.occluder = m_occluder.get();
                    maskInput.region_masker = m_region_masker.get();

                    cv::Mat composedMask = face::masker::MaskCompositor::compose(maskInput);

                    // 4. Paste back
                    working_frame = face::helper::paste_back(working_frame, enhanced_crop,
                                                             composedMask, affine_matrix);
                }

                // Global Face Blend
                if (input.face_blend >= 100) {
                    frame.image = working_frame;
                } else if (input.face_blend > 0) {
                    double alpha = input.face_blend / 100.0;
                    cv::addWeighted(working_frame, alpha, frame.image, 1.0 - alpha, 0.0,
                                    frame.image);
                }
                // if 0, do nothing

            } catch (const std::exception& e) {
                foundation::infrastructure::logger::Logger::get_instance()->error(
                    std::format("FaceEnhancerAdapter::process failed: {}", e.what()));
            }
        }
    }

private:
    std::shared_ptr<face::enhancer::IFaceEnhancer> m_enhancer;
    std::string m_model_path;
    foundation::ai::inference_session::Options m_options;
    bool m_loaded = false;
    std::mutex m_load_mutex;

    std::shared_ptr<face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<face::masker::IFaceRegionMasker> m_region_masker;

    cv::Size m_input_size{512, 512}; // Default, updated on load
    domain::face::helper::WarpTemplateType m_template_type =
        domain::face::helper::WarpTemplateType::Ffhq_512;
};

/**
 * @brief Adapter for Face Expression Restorer
 * @details Wraps IFaceExpressionRestorer to implement IFrameProcessor interface for the pipeline.
 */
export class ExpressionAdapter : public IFrameProcessor {
public:
    /**
     * @brief Construct a new Expression Adapter
     * @param context_ptr Pointer to
     * PipelineContext
     */
    static std::shared_ptr<IFrameProcessor> create(const void* context_ptr);

private:
    /**
     * @brief Internal constructor
     */
    explicit ExpressionAdapter(
        std::shared_ptr<face::expression::IFaceExpressionRestorer> restorer,
        std::string feature_path, std::string motion_path, std::string generator_path,
        foundation::ai::inference_session::Options options,
        std::shared_ptr<face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_restorer(std::move(restorer)), m_feature_path(std::move(feature_path)),
        m_motion_path(std::move(motion_path)), m_generator_path(std::move(generator_path)),
        m_options(std::move(options)), m_occluder(std::move(occluder)),
        m_region_masker(std::move(region_masker)) {}

public:
    /**
     * @brief Ensures the expression restorer models are loaded into memory
     */
    void ensure_loaded() override {
        if (m_loaded) return;
        std::lock_guard<std::mutex> lock(m_load_mutex);
        if (m_loaded) return;

        if (m_restorer && !m_feature_path.empty()) {
            m_restorer->load_model(m_feature_path, m_motion_path, m_generator_path, m_options);
            m_size = m_restorer->get_model_input_size();

            // Dynamic template selection
            if (m_size.width == 512) {
                m_template_type = domain::face::helper::WarpTemplateType::Ffhq_512;
            } else {
                // Default fallback (includes 256x256 which LivePortrait often uses)
                m_template_type = domain::face::helper::WarpTemplateType::Arcface_128_v2;
            }
        }
        m_loaded = true;
    }

    /**
     * @brief Process a single frame using the expression restorer
     * @param frame The frame data to be processed and updated
     */
    void process(FrameData& frame) override {
        foundation::infrastructure::logger::ScopedTimer timer("ExpressionAdapter::process");
        ensure_loaded();
        if (!m_restorer) return;

        if (frame.expression_input.has_value()) {
            try {
                auto& input = frame.expression_input.value();

                if (input.source_frame.empty()) return;
                if (input.target_landmarks.empty() || input.source_landmarks.empty()) return;

                cv::Mat working_frame = frame.image;
                size_t count =
                    std::min(input.target_landmarks.size(), input.source_landmarks.size());

                for (size_t i = 0; i < count; ++i) {
                    // 1. Warp Source
                    auto [source_crop, source_affine] = face::helper::warp_face_by_face_landmarks_5(
                        input.source_frame, input.source_landmarks[i], m_template_type, m_size);

                    // 2. Warp Target
                    auto [target_crop, target_affine] = face::helper::warp_face_by_face_landmarks_5(
                        working_frame, input.target_landmarks[i], m_template_type, m_size);

                    // 3. Inference
                    cv::Mat restored_crop = m_restorer->restore_expression(source_crop, target_crop,
                                                                           input.restore_factor);

                    if (restored_crop.empty()) continue;

                    // 4. Compose Mask
                    face::masker::MaskCompositor::CompositionInput maskInput;
                    maskInput.size = m_size;

                    // Construct options from input fields
                    domain::face::types::MaskOptions opts;
                    opts.mask_types.assign(input.mask_types.begin(), input.mask_types.end());
                    opts.box_mask_blur = input.box_mask_blur;
                    opts.box_mask_padding = input.box_mask_padding;

                    maskInput.options = opts;
                    maskInput.crop_frame = target_crop;
                    maskInput.occluder = m_occluder.get();
                    maskInput.region_masker = m_region_masker.get();

                    cv::Mat composedMask = face::masker::MaskCompositor::compose(maskInput);

                    // 5. Paste back
                    working_frame = face::helper::paste_back(working_frame, restored_crop,
                                                             composedMask, target_affine);
                }
                frame.image = working_frame;

            } catch (const std::exception& e) {
                foundation::infrastructure::logger::Logger::get_instance()->error(
                    std::format("ExpressionAdapter::process failed: {}", e.what()));
            }
        }
    }

private:
    std::shared_ptr<face::expression::IFaceExpressionRestorer> m_restorer;
    std::string m_feature_path;
    std::string m_motion_path;
    std::string m_generator_path;
    foundation::ai::inference_session::Options m_options;
    bool m_loaded = false;
    std::mutex m_load_mutex;

    std::shared_ptr<face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<face::masker::IFaceRegionMasker> m_region_masker;

    cv::Size m_size{512, 512};
    domain::face::helper::WarpTemplateType m_template_type =
        domain::face::helper::WarpTemplateType::Arcface_128_v2;
};

/**
 * @brief Adapter for Frame Enhancer
 * @details Wraps IFrameEnhancer to implement IFrameProcessor interface for the pipeline.
 */
export class FrameEnhancerAdapter : public IFrameProcessor {
public:
    /**
     * @brief Construct a new Frame Enhancer Adapter
     * @param context_ptr Pointer to
     * PipelineContext
     */
    static std::shared_ptr<IFrameProcessor> create(const void* context_ptr);

private:
    /**
     * @brief Internal constructor
     */
    explicit FrameEnhancerAdapter(
        std::function<std::shared_ptr<frame::enhancer::IFrameEnhancer>()> factory_func) :
        m_factory_func(std::move(factory_func)) {}

public:
    /**
     * @brief Ensures the frame enhancer is created and loaded
     */
    void ensure_loaded() override {
        if (m_loaded) return;
        std::lock_guard<std::mutex> lock(m_load_mutex);
        if (m_loaded) return;

        if (m_factory_func) { m_enhancer = m_factory_func(); }
        m_loaded = true;
    }

    // Force linking of adapter implementations
    static void force_link() {}

    /**
     * @brief Process a single frame using the frame enhancer
     * @param frame The frame data to be processed and updated
     */
    void process(FrameData& frame) override {
        foundation::infrastructure::logger::ScopedTimer timer("FrameEnhancerAdapter::process");
        ensure_loaded();
        if (!m_enhancer) return;

        frame::enhancer::FrameEnhancerInput input;
        input.target_frame = frame.image; // cv::Mat shallow copy (ref counted)

        if (frame.metadata.contains("frame_enhancer_blend")) {
            try {
                input.blend =
                    std::any_cast<unsigned short>(frame.metadata.at("frame_enhancer_blend"));
            } catch (const std::bad_any_cast& e) {
                foundation::infrastructure::logger::Logger::get_instance()->error(std::format(
                    "FrameEnhancerAdapter: Bad any cast for frame_enhancer_blend: {}", e.what()));
            } catch (...) {
                foundation::infrastructure::logger::Logger::get_instance()->error(
                    "FrameEnhancerAdapter: Unknown error reading frame_enhancer_blend");
            }
        }

        frame.image = m_enhancer->enhance_frame(input);
    }

private:
    std::shared_ptr<frame::enhancer::IFrameEnhancer> m_enhancer;
    std::function<std::shared_ptr<frame::enhancer::IFrameEnhancer>()> m_factory_func;
    bool m_loaded = false;
    std::mutex m_load_mutex;
};

} // namespace domain::pipeline
