module;
#include <opencv2/core/mat.hpp>
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <vector>

export module domain.face.detector:impl_base;

import :api;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

export namespace domain::face::detector {

class FaceDetectorImplBase : public IFaceDetector {
public:
    virtual ~FaceDetectorImplBase() = default;

    void load_model(const std::string& model_path, const InferenceOptions& options) override {
        m_session =
            foundation::ai::inference_session::InferenceSessionRegistry::get_instance().get_session(
                model_path, options);
    }

    [[nodiscard]] bool is_model_loaded() const { return m_session && m_session->is_model_loaded(); }

    [[nodiscard]] std::vector<std::vector<int64_t>> get_input_node_dims() const {
        if (!m_session) return {};
        return m_session->get_input_node_dims();
    }

    [[nodiscard]] std::vector<std::vector<int64_t>> get_output_node_dims() const {
        if (!m_session) return {};
        return m_session->get_output_node_dims();
    }

    std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors) {
        if (!m_session) return {};
        return m_session->run(input_tensors);
    }

protected:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
};
} // namespace domain::face::detector
