module;
#include <memory>
#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.swapper:impl_base;

import :api;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

export namespace domain::face::swapper {

class FaceSwapperImplBase : public IFaceSwapper {
public:
    virtual ~FaceSwapperImplBase() = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options = {}) override {
        m_session =
            foundation::ai::inference_session::InferenceSessionRegistry::get_instance().get_session(
                model_path, options);
    }

    [[nodiscard]] bool is_model_loaded() const { return m_session && m_session->is_model_loaded(); }

    [[nodiscard]] std::vector<std::vector<int64_t>> get_input_node_dims() const {
        if (!m_session) return {};
        return m_session->get_input_node_dims();
    }

    [[nodiscard]] std::string get_loaded_model_path() const {
        if (!m_session) return "";
        return m_session->get_loaded_model_path();
    }

    std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors) {
        if (!m_session) return {};
        return m_session->run(input_tensors);
    }

protected:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
    std::vector<const char*> m_input_names;
    std::vector<const char*> m_output_names;
    Ort::MemoryInfo m_memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
};

} // namespace domain::face::swapper
