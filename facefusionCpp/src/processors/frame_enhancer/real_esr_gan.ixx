/**
 ******************************************************************************
 * @file           : real_esr_gan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-30
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

export module frame_enhancer:real_esr_gan;
import :frame_enhancer_base;
import inference_session;

export namespace ffc::frame_enhancer {

struct RealEsrGanInput {
    std::shared_ptr<cv::Mat> target_frame{nullptr};
    unsigned short blend{80};
};

class RealEsrGan final : public FrameEnhancerBase, public ai::InferenceSession {
public:
    explicit RealEsrGan(const std::shared_ptr<Ort::Env>& env);
    ~RealEsrGan() override = default;

    [[nodiscard]] std::string get_processor_name() const override;

    [[nodiscard]] cv::Mat enhanceFrame(const RealEsrGanInput& input) const;
};
} // namespace ffc::frame_enhancer
