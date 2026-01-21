module;
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @file face_detector_impl_base.cpp
 * @brief Face detector implementation base class implementation
 * @author CodingRookie
 * @date 2026-01-21
 */
module domain.face.detector;

import :impl_base;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

namespace domain::face::detector {

void FaceDetectorImplBase::load_model(const std::string& model_path,
                                      const InferenceOptions& options) {
    m_session =
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance().get_session(
            model_path, options);
}

bool FaceDetectorImplBase::is_model_loaded() const {
    return m_session && m_session->is_model_loaded();
}

std::vector<std::vector<int64_t>> FaceDetectorImplBase::get_input_node_dims() const {
    if (!m_session) return {};
    return m_session->get_input_node_dims();
}

std::vector<std::vector<int64_t>> FaceDetectorImplBase::get_output_node_dims() const {
    if (!m_session) return {};
    return m_session->get_output_node_dims();
}

std::vector<Ort::Value> FaceDetectorImplBase::run(const std::vector<Ort::Value>& input_tensors) {
    if (!m_session) return {};
    return m_session->run(input_tensors);
}

} // namespace domain::face::detector
