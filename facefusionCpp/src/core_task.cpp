/**
 ******************************************************************************
 * @file           : core_run_options.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 25-2-20
 ******************************************************************************
 */

module;
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <format>
#include <fstream>
#include <opencv2/opencv.hpp>

module core;
import :core_task;
import processor_hub;
import vision;
import file_system;
import logger;

namespace ffc {
using namespace faceSwapper;
using namespace faceEnhancer;
using namespace expressionRestore;
using namespace frameEnhancer;

FaceSwapperInput
CoreTask::GetFaceSwapperInput(const size_t& target_paths_index,
                              const std::shared_ptr<FaceAnalyser>& face_analyser) {
    if (target_paths.empty()) {
        Logger::getInstance()->error("target_paths is empty");
        return {};
    }
    if (target_paths_index >= target_paths.size()) {
        Logger::getInstance()->error(std::format("target_paths_index is out of range! target_paths_index : {} , target_paths.size is {}", target_paths_index, target_paths.size()));
        return {};
    }

    FaceSwapperInput face_swapper_input;

    if (processor_minor_types.at(ProcessorMajorType::FaceSwapper).face_swapper.value()
        == FaceSwapperType::InSwapper) {
        face_swapper_input.in_swapper_input = std::make_unique<InSwapperInput>();
        face_swapper_input.in_swapper_input->target_frame = std::make_shared<cv::Mat>(vision::readStaticImage(target_paths.at(target_paths_index)));
        for (const std::vector<Face> target_faces = GetTargetFaces(*face_swapper_input.in_swapper_input->target_frame, face_analyser);
             const auto& face : target_faces) {
            face_swapper_input.in_swapper_input->target_faces_5_landmarks.emplace_back(face.m_landMark5From68);
        }
        face_swapper_input.in_swapper_input->args_for_get_best_mask = GetArgsForGetBestMask();

        if (source_average_face != nullptr) {
            face_swapper_input.in_swapper_input->source_average_embeddings = source_average_face->m_embedding;
        } else {
            if (face_analyser->GetFaceStore()->IsContains(source_average_face_id.value())) {
                face_swapper_input.in_swapper_input->source_average_embeddings = face_analyser->GetFaceStore()->GetFaces(source_average_face_id.value()).front().m_embedding;
            } else {
                face_swapper_input.in_swapper_input->source_average_embeddings = ProcessSourceAverageFace(face_analyser).m_embedding;
            }
        }
    }
    return face_swapper_input;
}

std::vector<Face>
CoreTask::GetTargetFaces(const cv::Mat& target_frame,
                         const std::shared_ptr<FaceAnalyser>& face_analyser) const {
    std::vector<Face> targetFaces;
    if (face_selector_mode.value() == FaceSelector::SelectorMode::Many) {
        targetFaces = face_analyser->GetManyFaces(target_frame, face_analyser_options.value());
    } else if (face_selector_mode.value() == FaceSelector::SelectorMode::One) {
        targetFaces.emplace_back(face_analyser->GetOneFace(target_frame, face_analyser_options.value(), reference_face_position.value()));
    } else {
        if (reference_face_path.value().empty()) {
            Logger::getInstance()->error(std::string(__FUNCTION__) + " reference_face_path is empty");
            return {};
        }
        if (FileSystem::isImage(reference_face_path.value())) {
            const cv::Mat refFrame = vision::readStaticImage(reference_face_path.value());
            const vector<Face> refFaces = face_analyser->GetManyFaces(refFrame, face_analyser_options.value());
            if (refFaces.empty()) {
                Logger::getInstance()->error(std::string(__FUNCTION__) + " reference_face is empty");
                return {};
            }
            const vector<Face> simFaces = face_analyser->FindSimilarFaces(refFaces, target_frame, reference_face_distance.value(), face_analyser_options.value());
            if (!simFaces.empty()) {
                targetFaces = simFaces;
            } else {
                Logger::getInstance()->error(std::string(__FUNCTION__) + " reference_face is empty");
            }

        } else {
            Logger::getInstance()->error(std::string(__FUNCTION__) + " reference_face_path is not a image file");
            return {};
        }
    }
    return targetFaces;
}

Face CoreTask::ProcessSourceAverageFace(const std::shared_ptr<FaceAnalyser>& face_analyser) const {
    if (face_analyser->GetFaceStore()->IsContains(source_average_face_id.value())) {
        return face_analyser->GetFaceStore()->GetFaces(source_average_face_id.value()).front();
    }

    static mutex mtx;
    std::lock_guard lock(mtx);
    // process source_average_face
    if (!source_paths.has_value()) {
        Logger::getInstance()->error(std::string(__FUNCTION__) + " source_paths is empty");
        return {};
    }
    std::vector<std::string> sourceImagePaths = source_paths.value();
    if (sourceImagePaths.empty()) {
        Logger::getInstance()->error(std::string(__FUNCTION__) + " source_paths is empty");
        return {};
    }

    std::unordered_set sourcePaths(sourceImagePaths.cbegin(), sourceImagePaths.cend());
    sourcePaths = FileSystem::filterImagePaths(sourcePaths);
    if (sourcePaths.empty()) {
        Logger::getInstance()->error(std::string(__FUNCTION__) + "The source path is a directory but the directory does not contain any image files!");
        return {};
    }

    sourcePaths = FileSystem::filterImagePaths(sourcePaths);
    if (!face_analyser_options.has_value()) {
        Logger::getInstance()->warn(std::string(__FUNCTION__) + " face_analyser_options is empty");
    }

    std::vector<cv::Mat> frames = vision::readStaticImages(sourcePaths, std::thread::hardware_concurrency() / 2);
    Face sourceAverageFace;
    if (face_analyser_options.has_value()) {
        sourceAverageFace = face_analyser->GetAverageFace(frames, face_analyser_options.value());
    } else {
        sourceAverageFace = face_analyser->GetAverageFace(frames, {});
    }
    frames.clear();
    if (sourceAverageFace.isEmpty()) {
        Logger::getInstance()->error(std::string(__FUNCTION__) + " source face is empty");
        return {};
    }
    face_analyser->GetFaceStore()->InsertFaces(source_average_face_id.value(), {sourceAverageFace});
    return sourceAverageFace;
}

FaceEnhancerInput
CoreTask::GetFaceEnhancerInput(const size_t& target_paths_index,
                               const std::shared_ptr<FaceAnalyser>& face_analyser) const {
    if (target_paths.empty()) {
        Logger::getInstance()->error("target_paths is empty");
        return {};
    }
    if (target_paths_index >= target_paths.size()) {
        Logger::getInstance()->error(std::format("target_paths_index is out of range! target_paths_index : {} , target_paths.size is {}", target_paths_index, target_paths.size()));
        return {};
    }

    FaceEnhancerInput face_enhancer_input;
    const auto target_frame = std::make_shared<cv::Mat>(vision::readStaticImage(target_paths.at(target_paths_index)));

    if (processor_minor_types.at(ProcessorMajorType::FaceEnhancer).face_enhancer.value()
        == FaceEnhancerType::CodeFormer) {
        face_enhancer_input.code_former_input = std::make_unique<CodeFormerInput>();
        face_enhancer_input.code_former_input->target_frame = target_frame;
        for (const std::vector<Face> target_faces = GetTargetFaces(*face_enhancer_input.code_former_input->target_frame, face_analyser);
             const auto& face : target_faces) {
            face_enhancer_input.code_former_input->target_faces_5_landmarks.emplace_back(face.m_landMark5From68);
        }
        face_enhancer_input.code_former_input->args_for_get_best_mask = GetArgsForGetBestMask();
    }
    if (processor_minor_types.at(ProcessorMajorType::FaceEnhancer).face_enhancer.value()
        == FaceEnhancerType::GFP_GAN) {
        face_enhancer_input.gfp_gan_input = std::make_unique<GFP_GAN_Input>();
        face_enhancer_input.gfp_gan_input->target_frame = target_frame;
        for (const std::vector<Face> target_faces = GetTargetFaces(*face_enhancer_input.gfp_gan_input->target_frame, face_analyser);
             const auto& face : target_faces) {
            face_enhancer_input.gfp_gan_input->target_faces_5_landmarks.emplace_back(face.m_landMark5From68);
        }
        face_enhancer_input.gfp_gan_input->args_for_get_best_mask = GetArgsForGetBestMask();
    }

    return face_enhancer_input;
}

ExpressionRestorerInput
CoreTask::GetExpressionRestorerInput(const size_t& source_paths_index,
                                     const size_t& target_paths_index,
                                     const std::shared_ptr<FaceAnalyser>& face_analyser) const {
    if (target_paths.empty()) {
        Logger::getInstance()->error("target_paths is empty");
        return {};
    }
    if (target_paths_index >= target_paths.size()) {
        Logger::getInstance()->error(std::format("target_paths_index is out of range! target_paths_index : {}, target_paths.size is {}", target_paths_index, target_paths.size()));
        return {};
    }
    if (source_paths.value().empty()) {
        Logger::getInstance()->error("source_paths is empty");
        return {};
    }
    if (source_paths_index >= source_paths.value().size()) {
        Logger::getInstance()->error(std::format("source_paths_index is out of range! source_paths_index : {}, source_paths.size is {}", source_paths_index, source_paths.value().size()));
        return {};
    }

    ExpressionRestorerInput expression_restorer_input;

    const auto source_frame = std::make_shared<cv::Mat>(vision::readStaticImage(source_paths.value().at(source_paths_index)));
    const auto target_frame = std::make_shared<cv::Mat>(vision::readStaticImage(target_paths.at(target_paths_index)));

    if (processor_minor_types.at(ProcessorMajorType::ExpressionRestorer).expression_restorer.value()
        == ExpressionRestorerType::LivePortrait) {
        expression_restorer_input.live_portrait_input = std::make_unique<LivePortraitInput>();
        expression_restorer_input.live_portrait_input->source_frame = source_frame;
        expression_restorer_input.live_portrait_input->target_frame = target_frame;
        for (const std::vector<Face> source_faces = GetTargetFaces(*expression_restorer_input.live_portrait_input->source_frame, face_analyser);
             const auto& face : source_faces) {
            expression_restorer_input.live_portrait_input->source_faces_5_landmarks.emplace_back(face.m_landMark5From68);
        }
        for (const std::vector<Face> target_faces = GetTargetFaces(*expression_restorer_input.live_portrait_input->target_frame, face_analyser);
             const auto& face : target_faces) {
            expression_restorer_input.live_portrait_input->target_faces_5_landmarks.emplace_back(face.m_landMark5From68);
        }
        expression_restorer_input.live_portrait_input->restoreFactor = expression_restorer_factor.value();
        if (face_mask_types.value().contains(FaceMaskerHub::Type::Occlusion)) {
            expression_restorer_input.live_portrait_input->faceMaskersTypes.insert(FaceMaskerHub::Type::Occlusion);
        }
        expression_restorer_input.live_portrait_input->boxMaskBlur = face_mask_blur.value();
        expression_restorer_input.live_portrait_input->boxMaskPadding = face_mask_padding.value();
    }

    return expression_restorer_input;
}

FrameEnhancerInput
CoreTask::GetFrameEnhancerInput(const size_t& target_paths_index) const {
    if (target_paths.empty()) {
        Logger::getInstance()->error("target_paths is empty");
        return {};
    }
    if (target_paths_index >= target_paths.size()) {
        Logger::getInstance()->error(std::format("target_paths_index is out of range! target_paths_index : {}, target_paths.size is {}", target_paths_index, target_paths.size()));
        return {};
    }

    FrameEnhancerInput frame_enhancer_input;

    if (processor_minor_types.at(ProcessorMajorType::FrameEnhancer).frame_enhancer.value()
        == FrameEnhancerType::Real_esr_gan) {
        frame_enhancer_input.real_esr_gan_input = std::make_unique<RealEsrGanInput>();
        frame_enhancer_input.real_esr_gan_input->target_frame = std::make_shared<cv::Mat>(vision::readStaticImage(target_paths.at(target_paths_index)));
        frame_enhancer_input.real_esr_gan_input->blend = frame_enhancer_blend.value();
    }
    if (processor_minor_types.at(ProcessorMajorType::FrameEnhancer).frame_enhancer.value()
        == FrameEnhancerType::Real_hat_gan) {
        frame_enhancer_input.real_hat_gan_input = std::make_unique<RealHatGanInput>();
        frame_enhancer_input.real_hat_gan_input->target_frame = std::make_shared<cv::Mat>(vision::readStaticImage(target_paths.at(target_paths_index)));
        frame_enhancer_input.real_hat_gan_input->blend = frame_enhancer_blend.value();
    }

    return frame_enhancer_input;
}

} // namespace ffc