module;
#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.enhancer:impl_base;
import :api;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

export namespace domain::face::enhancer {

class FaceEnhancerImplBase : public IFaceEnhancer {
public:
    virtual ~FaceEnhancerImplBase() = default;

    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options) override {
        m_session = foundation::ai::inference_session::InferenceSessionRegistry::get_instance()
                        ->get_session(model_path, options);
    }

    [[nodiscard]] bool is_model_loaded() const { return m_session && m_session->is_model_loaded(); }

protected:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
    std::vector<const char*> m_input_names;
    std::vector<const char*> m_output_names;
    Ort::MemoryInfo m_memory_info =
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    [[nodiscard]] static cv::Mat blend_frame(const cv::Mat& target_frame,
                                             const cv::Mat& paste_vision_frame,
                                             std::uint16_t blend) {
        if (blend > 100) { blend = 100; }
        const float face_enhancer_blend = 1.0F - (static_cast<float>(blend) / 100.0F);
        cv::Mat dst_image;
        cv::addWeighted(target_frame, face_enhancer_blend, paste_vision_frame,
                        1.0F - face_enhancer_blend, 0, dst_image);
        return dst_image;
    }
};

} // namespace domain::face::enhancer
