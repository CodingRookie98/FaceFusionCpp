module;
#include <opencv2/core/mat.hpp>
#include <unordered_set>
#include <vector>

export module domain.face.masker:impl_region;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

class RegionMasker final : public IFaceRegionMasker,
                           public foundation::ai::inference_session::InferenceSession {
public:
    using foundation::ai::inference_session::InferenceSession::InferenceSession;
    ~RegionMasker() override = default;

    cv::Mat create_region_mask(const cv::Mat& crop_vision_frame,
                               const std::unordered_set<FaceRegion>& regions) override;
};

} // namespace domain::face::masker
