/**
 ******************************************************************************
 * @file           : task.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-12-31
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module core:core_run_options;
import model_manager;
import face_selector;
import face_masker_hub;
import face_analyser;
import processor_hub;

using namespace ffc::faceMasker;

namespace ffc {
export class CoreRunOptions {
public:
    // Processors
    std::list<ProcessorMajorType> processor_list;
    std::unordered_map<ProcessorMajorType, ProcessorMinorType> processor_minor_types;
    std::unordered_map<ProcessorMajorType, ModelManager::Model> processor_model;
    std::optional<unsigned short> face_enhancer_blend;
    std::optional<float> expression_restorer_factor;
    std::optional<unsigned short> frame_enhancer_blend;

    // general
    std::optional<std::vector<std::string>> source_paths;
    std::vector<std::string> target_paths;
    std::optional<std::string> reference_face_path;
    std::vector<std::string> output_paths;

    // face selector
    std::optional<FaceSelector::SelectorMode> face_selector_mode;
    std::optional<unsigned short> reference_face_position;
    std::optional<float> reference_face_distance;
    std::optional<unsigned long long> reference_frame_number;

    std::optional<FaceAnalyser::Options> face_analyser_options;

    // face masker
    std::optional<std::unordered_set<FaceMaskerHub::Type>> face_mask_types;
    std::optional<float> face_mask_blur;
    std::optional<std::array<int, 4>> face_mask_padding;
    std::optional<std::unordered_set<FaceMaskerRegion::Region>> face_mask_regions;

    // output creation
    std::optional<unsigned short> output_image_quality;
    std::optional<cv::Size> output_image_size;

    // video
    std::optional<unsigned long> video_segment_duration;
    std::optional<std::string> output_video_encoder;
    std::optional<std::string> output_video_preset;
    std::optional<unsigned short> output_video_quality;
    std::optional<std::string> output_audio_encoder;
    std::optional<bool> skip_audio;
    std::optional<std::string> temp_frame_format;

    // extra options
    std::optional<std::string> source_average_face_id;
    bool show_progress_bar = true;
};
} // namespace ffc