module;
#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>

export module domain.frame.enhancer:types;

export namespace domain::frame::enhancer {

enum class FrameEnhancerType { RealEsrGan, RealHatGan };

struct FrameEnhancerInput {
    std::shared_ptr<cv::Mat> target_frame{nullptr};
    unsigned short blend{80};
};

} // namespace domain::frame::enhancer
