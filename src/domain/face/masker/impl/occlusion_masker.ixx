module;
#include <opencv2/core/mat.hpp>
#include <vector>

export module domain.face.masker:impl_occlusion;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

class OcclusionMasker final : public IFaceOccluder,
                              public foundation::ai::inference_session::InferenceSession {
public:
    using foundation::ai::inference_session::InferenceSession::InferenceSession;
    ~OcclusionMasker() override = default;

    cv::Mat create_occlusion_mask(const cv::Mat& crop_vision_frame) override;
};

} // namespace domain::face::masker
