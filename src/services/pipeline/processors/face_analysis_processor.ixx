/**
 * @file face_analysis_processor.ixx
 * @brief Processor for high-level face analysis in the pipeline
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <vector>
#include <memory>
#include <unordered_set>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <utility>

export module services.pipeline.processors.face_analysis;

import domain.pipeline;
import domain.face.analyser;
import domain.face.swapper;
import domain.face.enhancer;
import domain.face.expression;
import domain.face.helper;
import domain.face.masker;
import config.task; // FaceMaskerConfig
import foundation.infrastructure.logger;
import services.pipeline.metrics;

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
     * @param metrics Optional metrics collector
     * @param masker_config Face masker configuration（types 含 occlusion/region 时启用共享 mask）
     * @param occluder Occlusion masker（可空）
     * @param region_masker Region masker（可空）
     */
    FaceAnalysisProcessor(
        std::shared_ptr<domain::face::analyser::FaceAnalyser> analyser,
        std::shared_ptr<const std::vector<float>> src_emb, FaceAnalysisRequirements reqs,
        MetricsCollector* metrics = nullptr, config::FaceMaskerConfig masker_config = {},
        std::shared_ptr<domain::face::masker::IFaceOccluder> occluder = nullptr,
        std::shared_ptr<domain::face::masker::IFaceRegionMasker> region_masker = nullptr) :
        m_analyser(std::move(analyser)), source_embedding(std::move(src_emb)), m_reqs(reqs),
        m_metrics(metrics), m_masker_config(std::move(masker_config)),
        m_occluder(std::move(occluder)), m_region_masker(std::move(region_masker)) {}

    /**
     * @brief Detect faces and attach processing metadata to the frame
     */
    void process(FrameData& frame) override {
        std::unique_ptr<ScopedStepTimer> timer;
        if (m_metrics) { timer = std::make_unique<ScopedStepTimer>(*m_metrics, "face_analysis"); }

        auto faces = m_analyser->get_many_faces(
            frame.image, domain::face::analyser::FaceAnalysisType::Detection);

        if (faces.empty()) {
            // [E403] 必须记录 WARN 日志 (design.md Section 5.3.2)
            foundation::infrastructure::logger::Logger::get_instance()->warn(std::format(
                "[E403] No face detected in frame {}, passing through", frame.sequence_id));

            // 记录 metrics: 跳过帧
            if (m_metrics) { m_metrics->record_frame_skipped(); }
            return; // 透传帧，不修改
        }

        // 0. 共享 mask：masker 启用时计算一次，供 swapper/enhancer adapter 缩放复用
        if (m_masker_enabled()) { compose_shared_masks(frame, faces); }

        // 1. Prepare Swap Data
        if (m_reqs.need_swap_data) {
            domain::face::swapper::SwapInput swap_input;
            // swap_input.target_frame = frame.image; // Removed in refactoring
            for (const auto& face : faces) {
                swap_input.target_faces_landmarks.push_back(face.get_landmark5());
            }
            swap_input.source_embedding = source_embedding;
            frame.swap_input = std::move(swap_input);
        }

        // 2. Prepare Enhance Data
        if (m_reqs.need_enhance_data) {
            domain::face::enhancer::EnhanceInput enhance_input;
            // enhance_input.target_frame = frame.image; // Removed in refactoring
            for (const auto& face : faces) {
                enhance_input.target_faces_landmarks.push_back(face.get_landmark5());
            }
            enhance_input.face_blend = 80; // Default blend factor
            frame.enhance_input = std::move(enhance_input);
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
            frame.expression_input = std::move(expression_input);
        }
    }

private:
    bool m_masker_enabled() const {
        if (!m_occluder && !m_region_masker) return false;
        for (const auto& t : m_masker_config.types) {
            if (t != "box") return true;
        }
        return false;
    }

    // 计算每张脸的共享组合 mask（512 参考尺度），存 FrameData 供 adapter 复用
    void compose_shared_masks(FrameData& frame, const std::vector<domain::face::Face>& faces) {
        using namespace domain::face;
        namespace helper = domain::face::helper;

        FaceMaskCache cache;
        cache.masks.reserve(faces.size());
        cache.reference_size = 512;

        const auto kOptions = build_mask_options();
        for (const auto& face : faces) {
            auto [crop, affine] = helper::warp_face_by_face_landmarks_5(
                frame.image, face.get_landmark5(), helper::WarpTemplateType::Ffhq512,
                cv::Size{512, 512});

            masker::MaskCompositor::CompositionInput input;
            input.size = {512, 512};
            input.options = kOptions;
            input.crop_frame = crop;
            input.occluder = m_occluder.get();
            input.region_masker = m_region_masker.get();
            cache.masks.push_back(masker::MaskCompositor::compose(input));
        }
        frame.mask_cache = std::move(cache);
    }

    domain::face::types::MaskOptions build_mask_options() const {
        using namespace domain::face::types;
        MaskOptions options;
        for (const auto& t : m_masker_config.types) {
            if (t == "box") {
                if (std::find(options.mask_types.begin(), options.mask_types.end(), MaskType::Box)
                    == options.mask_types.end()) {
                    options.mask_types.push_back(MaskType::Box);
                }
            } else if (t == "occlusion") {
                options.mask_types.push_back(MaskType::Occlusion);
            } else if (t == "region") {
                options.mask_types.push_back(MaskType::Region);
            }
        }
        if (options.mask_types.empty()) { options.mask_types.push_back(MaskType::Box); }

        // region 配置映射："face"=全部面部区域，"eyes"=双眼；空 = 全部面部区域
        std::unordered_set<std::string> regions(m_masker_config.region.begin(),
                                                m_masker_config.region.end());
        if (regions.empty() || regions.contains("face")) {
            options.regions = {
                FaceRegion::Skin,         FaceRegion::Nose,     FaceRegion::LeftEyebrow,
                FaceRegion::RightEyebrow, FaceRegion::LeftEye,  FaceRegion::RightEye,
                FaceRegion::Mouth,        FaceRegion::UpperLip, FaceRegion::LowerLip};
        } else if (regions.contains("eyes")) {
            options.regions = {FaceRegion::LeftEye, FaceRegion::RightEye};
        }
        return options;
    }

    std::shared_ptr<domain::face::analyser::FaceAnalyser> m_analyser;
    std::shared_ptr<const std::vector<float>> source_embedding;
    FaceAnalysisRequirements m_reqs;
    MetricsCollector* m_metrics = nullptr;
    config::FaceMaskerConfig m_masker_config;
    std::shared_ptr<domain::face::masker::IFaceOccluder> m_occluder;
    std::shared_ptr<domain::face::masker::IFaceRegionMasker> m_region_masker;
};

} // namespace services::pipeline::processors
