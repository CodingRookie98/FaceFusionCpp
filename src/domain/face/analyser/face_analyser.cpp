module;
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <tuple>
#include <numeric>
#include <opencv2/opencv.hpp>

module domain.face.analyser;

import domain.face;
import domain.face.model_registry;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.selector;
import domain.face.store;
import domain.face.helper;
import domain.common;
import foundation.infrastructure.logger;

namespace domain::face::analyser {

using namespace domain::face::detector;
using namespace domain::face::landmarker;
using namespace domain::face::recognizer;
using namespace domain::face::classifier;
using namespace domain::face::store;
using foundation::infrastructure::logger::Logger;
using foundation::infrastructure::logger::LogLevel;
using foundation::infrastructure::logger::ScopedTimer;

class FaceAnalyser::Impl {
public:
    Impl(const Options& options) : m_options(options) {
        apply_options(options);
        m_face_store = FaceStore::get_instance();
    }

    Impl(const Options& options, std::shared_ptr<IFaceDetector> detector,
         std::shared_ptr<IFaceLandmarker> landmarker, std::shared_ptr<FaceRecognizer> recognizer,
         std::shared_ptr<IFaceClassifier> classifier, std::shared_ptr<FaceStore> store) :
        m_options(options), m_detector(std::move(detector)), m_landmarker(std::move(landmarker)),
        m_recognizer(std::move(recognizer)), m_classifier(std::move(classifier)) {
        if (store) m_face_store = std::move(store);
        else m_face_store = FaceStore::get_instance();
    }

    void update_options(const Options& options) { apply_options(options); }

    std::vector<Face> get_many_faces(const cv::Mat& vision_frame, FaceAnalysisType type) {
        ScopedTimer timer("FaceAnalyser::get_many_faces", LogLevel::Debug);
        auto& logger = *Logger::get_instance();

        if (vision_frame.empty()) {
            logger.warn("FaceAnalyser: Empty vision frame received.");
            return {};
        }

        std::vector<DetectionResult> detection_results;
        double detected_angle = 0;

        // 1. Check Cache
        if (m_face_store->is_contains(vision_frame)) {
            auto cached_faces = m_face_store->get_faces(vision_frame);
            bool cache_satisfies = true;

            if (!cached_faces.empty()) {
                const auto& face = cached_faces[0];
                if (has_flag(type, FaceAnalysisType::Embedding) && face.embedding().empty()) {
                    cache_satisfies = false;
                }
                if (has_flag(type, FaceAnalysisType::Landmark) && face.kps().empty()) {
                    cache_satisfies = false;
                }
                if (has_flag(type, FaceAnalysisType::GenderAge)) {
                    if (face.age_range().min == 0 && face.age_range().max == 100) {
                        cache_satisfies = false;
                    }
                }
            }

            if (cache_satisfies) {
                logger.debug("FaceAnalyser: Cache hit and satisfies requirements.");
                return cached_faces;
            }

            logger.debug("FaceAnalyser: Cache hit but insufficient data. Re-detecting.");
            detection_results.reserve(cached_faces.size());
            for (const auto& f : cached_faces) {
                DetectionResult res;
                res.box = f.box();
                res.score = f.detector_score();
                res.landmarks = f.get_landmark5();
                detection_results.push_back(res);
            }
        }

        // 2. Run Detector if not recovered from cache
        if (detection_results.empty()) {
            if (!m_detector) {
                logger.error("FaceAnalyser: Detector not initialized.");
                return {};
            }

            std::vector<int> angles = {0, 90, 180, 270};
            for (int angle : angles) {
                cv::Mat frame_to_detect;
                if (angle == 0) {
                    frame_to_detect = vision_frame;
                } else {
                    domain::face::helper::rotate_image_90n(vision_frame, frame_to_detect, angle);
                }

                auto results = m_detector->detect(frame_to_detect);

                int valid_count = 0;
                for (const auto& r : results) {
                    if (r.score >= m_options.face_detector_options.min_score) valid_count++;
                }

                if (valid_count > 0) {
                    detected_angle = angle;
                    detection_results = std::move(results);
                    logger.debug("FaceAnalyser: Faces detected at angle " + std::to_string(angle));
                    break;
                }
            }
        }

        if (detection_results.empty()) {
            logger.debug("FaceAnalyser: No faces detected.");
            m_face_store->insert_faces(vision_frame, {});
            return {};
        }

        auto result_faces = create_faces(vision_frame, detection_results, detected_angle, type);

        // Merge with cached faces
        if (!detection_results.empty() && m_face_store->is_contains(vision_frame)) {
            auto cached_faces = m_face_store->get_faces(vision_frame);
            if (cached_faces.size() == result_faces.size()) {
                for (size_t i = 0; i < result_faces.size(); ++i) {
                    auto& new_face = result_faces[i];
                    const auto& old_face = cached_faces[i];

                    if (new_face.embedding().empty() && !old_face.embedding().empty()) {
                        new_face.set_embedding(old_face.embedding());
                        new_face.set_normed_embedding(old_face.normed_embedding());
                    }

                    if (new_face.kps().size() == 5 && old_face.kps().size() > 5) {
                        new_face.set_kps(old_face.kps());
                        new_face.set_landmarker_score(old_face.landmarker_score());
                    }

                    if (new_face.gender() == domain::common::types::Gender::Male
                        && old_face.gender() != domain::common::types::Gender::Male
                        && !has_flag(type, FaceAnalysisType::GenderAge)) {
                        new_face.set_gender(old_face.gender());
                        new_face.set_age_range(old_face.age_range());
                        new_face.set_race(old_face.race());
                    }
                }
            }
        }

        m_face_store->insert_faces(vision_frame, result_faces);

        if (result_faces.empty()) return {};

        return selector::select_faces(result_faces, m_options.face_selector_options);
    }

private:
    void apply_options(const Options& options) {
        auto& registry = FaceModelRegistry::get_instance();

        // Detector
        auto get_det_path = [](const Options& opts) {
            switch (opts.face_detector_options.type) {
            case DetectorType::Yolo: return opts.model_paths.face_detector_yolo;
            case DetectorType::SCRFD: return opts.model_paths.face_detector_scrfd;
            case DetectorType::RetinaFace: return opts.model_paths.face_detector_retina;
            default: return std::string();
            }
        };
        std::string det_path = get_det_path(options);
        std::string old_det_path = get_det_path(m_options);

        if (!m_detector
            || options.face_detector_options.type != m_options.face_detector_options.type
            || det_path != old_det_path
            || options.inference_session_options != m_options.inference_session_options) {
            m_detector = registry.get_detector(options.face_detector_options.type, det_path,
                                               options.inference_session_options);
        }

        // Landmarker
        auto get_lm_path = [](const Options& opts) {
            switch (opts.face_landmarker_options.type) {
            case LandmarkerType::_2DFAN: return opts.model_paths.face_landmarker_2dfan;
            case LandmarkerType::Peppawutz: return opts.model_paths.face_landmarker_peppawutz;
            case LandmarkerType::_68By5: return opts.model_paths.face_landmarker_68by5;
            default: return std::string();
            }
        };
        std::string lm_path = get_lm_path(options);
        std::string old_lm_path = get_lm_path(m_options);

        if (!m_landmarker
            || options.face_landmarker_options.type != m_options.face_landmarker_options.type
            || lm_path != old_lm_path
            || options.inference_session_options != m_options.inference_session_options) {
            m_landmarker = registry.get_landmarker(options.face_landmarker_options.type, lm_path,
                                                   options.inference_session_options);
        }

        // Recognizer
        if (!m_recognizer || options.face_recognizer_type != m_options.face_recognizer_type
            || options.model_paths.face_recognizer_arcface
                   != m_options.model_paths.face_recognizer_arcface
            || options.inference_session_options != m_options.inference_session_options) {
            m_recognizer = registry.get_recognizer(options.face_recognizer_type,
                                                   options.model_paths.face_recognizer_arcface,
                                                   options.inference_session_options);
        }

        // Classifier
        if (!m_classifier || options.face_classifier_type != m_options.face_classifier_type
            || options.model_paths.face_classifier_fairface
                   != m_options.model_paths.face_classifier_fairface
            || options.inference_session_options != m_options.inference_session_options) {
            m_classifier = registry.get_classifier(options.face_classifier_type,
                                                   options.model_paths.face_classifier_fairface,
                                                   options.inference_session_options);
        }

        m_options = options;
    }

    std::vector<Face> create_faces(const cv::Mat& vision_frame,
                                   const std::vector<DetectionResult>& detection_results,
                                   double detected_angle, FaceAnalysisType type) {
        std::vector<Face> faces;
        if (detection_results.empty()) return faces;

        std::vector<cv::Rect2f> boxes;
        std::vector<float> scores;
        for (const auto& res : detection_results) {
            if (res.score >= m_options.face_detector_options.min_score) {
                boxes.push_back(res.box);
                scores.push_back(res.score);
            }
        }

        if (boxes.empty()) return faces;

        auto keep_indices = domain::face::helper::apply_nms(
            boxes, scores, m_options.face_detector_options.iou_threshold);

        cv::Mat rotated_frame;
        // Optimization: Only rotate if we are going to use landmarker.
        if (has_flag(type, FaceAnalysisType::Landmark) && detected_angle != 0) {
            domain::face::helper::rotate_image_90n(vision_frame, rotated_frame,
                                                   static_cast<int>(detected_angle));
        } else if (has_flag(type, FaceAnalysisType::Landmark)) {
            rotated_frame = vision_frame;
        }

        cv::Size original_size = vision_frame.size();

        std::vector<size_t> original_indices;
        for (size_t i = 0; i < detection_results.size(); ++i) {
            if (detection_results[i].score >= m_options.face_detector_options.min_score) {
                original_indices.push_back(i);
            }
        }

        for (int k_idx : keep_indices) {
            size_t original_idx = original_indices[k_idx];
            const auto& res = detection_results[original_idx];

            Face face;
            face.set_detector_score(res.score);

            domain::face::types::Landmarks kps5_back;
            for (const auto& p : res.landmarks) {
                kps5_back.push_back(domain::face::helper::rotate_point_back(
                    p, static_cast<int>(detected_angle), original_size));
            }

            face.set_box(domain::face::helper::rotate_box_back(
                res.box, static_cast<int>(detected_angle), original_size));
            face.set_kps(kps5_back);

            // Landmarking
            if (has_flag(type, FaceAnalysisType::Landmark)
                && m_options.face_landmarker_options.min_score > 0 && m_landmarker) {
                if (m_options.face_landmarker_options.type == landmarker::LandmarkerType::_68By5) {
                    auto kps68_back = m_landmarker->expand_68_from_5(kps5_back);
                    if (!kps68_back.empty()) {
                        face.set_kps(kps68_back);
                        face.set_landmarker_score(1.0f);
                    }
                } else {
                    auto lm_res = m_landmarker->detect(rotated_frame, res.box);
                    face.set_landmarker_score(lm_res.score);

                    if (lm_res.score > m_options.face_landmarker_options.min_score) {
                        domain::face::types::Landmarks kps68_back;
                        for (const auto& p : lm_res.landmarks) {
                            kps68_back.push_back(domain::face::helper::rotate_point_back(
                                p, static_cast<int>(detected_angle), original_size));
                        }
                        face.set_kps(kps68_back);
                    }
                }
            }

            auto kps5 = face.get_landmark5();

            // Recognition
            if (has_flag(type, FaceAnalysisType::Embedding) && m_recognizer) {
                auto [emb, norm_emb] = m_recognizer->recognize(vision_frame, kps5);
                face.set_embedding(emb);
                face.set_normed_embedding(norm_emb);
            }

            // Classification
            if (has_flag(type, FaceAnalysisType::GenderAge) && m_classifier) {
                auto class_res = m_classifier->classify(vision_frame, kps5);
                face.set_race(class_res.race);
                face.set_gender(class_res.gender);
                face.set_age_range(class_res.age);
            }

            faces.push_back(face);
        }

        return faces;
    }

    Options m_options;
    std::shared_ptr<IFaceDetector> m_detector;
    std::shared_ptr<IFaceLandmarker> m_landmarker;
    std::shared_ptr<FaceRecognizer> m_recognizer;
    std::shared_ptr<IFaceClassifier> m_classifier;
    std::shared_ptr<FaceStore> m_face_store;
};

// FaceAnalyser Implementation using PIMPL

FaceAnalyser::FaceAnalyser(const Options& options) : m_impl(std::make_unique<Impl>(options)) {}

FaceAnalyser::FaceAnalyser(const Options& options, std::shared_ptr<IFaceDetector> detector,
                           std::shared_ptr<IFaceLandmarker> landmarker,
                           std::shared_ptr<FaceRecognizer> recognizer,
                           std::shared_ptr<IFaceClassifier> classifier,
                           std::shared_ptr<FaceStore> store) :
    m_impl(std::make_unique<Impl>(options, std::move(detector), std::move(landmarker),
                                  std::move(recognizer), std::move(classifier), std::move(store))) {
}

FaceAnalyser::~FaceAnalyser() = default;

void FaceAnalyser::update_options(const Options& options) {
    m_impl->update_options(options);
}

std::vector<Face> FaceAnalyser::get_many_faces(const cv::Mat& vision_frame, FaceAnalysisType type) {
    return m_impl->get_many_faces(vision_frame, type);
}

Face FaceAnalyser::get_one_face(const cv::Mat& vision_frame, unsigned int position,
                                FaceAnalysisType type) {
    auto faces = get_many_faces(vision_frame, type);
    if (faces.empty()) return {};
    if (position >= faces.size()) return faces.back();
    return faces[position];
}

Face FaceAnalyser::get_average_face(const std::vector<cv::Mat>& vision_frames) {
    if (vision_frames.empty()) return {};
    std::vector<Face> all_faces;
    for (const auto& frame : vision_frames) {
        auto fs = get_many_faces(frame);
        all_faces.insert(all_faces.end(), fs.begin(), fs.end());
    }
    if (all_faces.empty()) return {};
    return get_average_face(all_faces);
}

Face FaceAnalyser::get_average_face(const std::vector<Face>& faces) {
    if (faces.empty()) return {};

    auto it = std::find_if(faces.begin(), faces.end(), [](const Face& f) { return !f.is_empty(); });
    if (it == faces.end()) return {};

    Face average_face = *it;

    if (faces.size() > 1) {
        std::vector<std::vector<float>> embeddings;
        std::vector<std::vector<float>> norm_embeddings;

        for (const auto& f : faces) {
            if (!f.embedding().empty()) embeddings.push_back(f.embedding());
            if (!f.normed_embedding().empty()) norm_embeddings.push_back(f.normed_embedding());
        }

        if (!embeddings.empty())
            average_face.set_embedding(domain::face::helper::calc_average_embedding(embeddings));

        if (!norm_embeddings.empty())
            average_face.set_normed_embedding(
                domain::face::helper::calc_average_embedding(norm_embeddings));
    }

    return average_face;
}

std::vector<Face> FaceAnalyser::find_similar_faces(const std::vector<Face>& reference_faces,
                                                   const cv::Mat& target_vision_frame,
                                                   float face_distance) {
    std::vector<Face> similar_faces;
    auto many_faces = get_many_faces(target_vision_frame);
    if (many_faces.empty()) return similar_faces;

    for (const auto& ref_face : reference_faces) {
        for (const auto& face : many_faces) {
            if (compare_face(face, ref_face, face_distance)) { similar_faces.push_back(face); }
        }
    }
    return similar_faces;
}

bool FaceAnalyser::compare_face(const Face& face, const Face& reference_face, float face_distance) {
    return calculate_face_distance(face, reference_face) < face_distance;
}

float FaceAnalyser::calculate_face_distance(const Face& face1, const Face& face2) {
    if (face1.normed_embedding().empty() || face2.normed_embedding().empty()) return 0.0f;

    float dot_product =
        std::inner_product(face1.normed_embedding().begin(), face1.normed_embedding().end(),
                           face2.normed_embedding().begin(), 0.0f);
    return 1.0f - dot_product;
}

} // namespace domain::face::analyser
