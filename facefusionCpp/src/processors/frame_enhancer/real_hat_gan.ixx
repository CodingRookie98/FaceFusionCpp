/**
 ******************************************************************************
 * @file           : real_hat_gan.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-31
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

export module frame_enhancer:real_hat_gan;
import :frame_enhancer_base;
import inference_session;

export namespace ffc::frameEnhancer {
struct RealHatGanInput {
    std::shared_ptr<cv::Mat> target_frame{nullptr};
    unsigned short blend{80};
};

class RealHatGan final : public FrameEnhancerBase, public InferenceSession {
public:
    explicit RealHatGan(const std::shared_ptr<Ort::Env>& env);
    ~RealHatGan() override = default;

    [[nodiscard]] std::string getProcessorName() const override;

    [[nodiscard]] cv::Mat enhanceFrame(const RealHatGanInput& input) const;
};
} // namespace ffc::frameEnhancer
