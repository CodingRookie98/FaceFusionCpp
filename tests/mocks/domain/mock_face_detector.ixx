module;
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <opencv2/core.hpp>

export module tests.mocks.domain.mock_face_detector;

import domain.face.detector;
import foundation.ai.inference_session;

export namespace tests::mocks::domain {

class MockFaceDetector : public ::domain::face::detector::IFaceDetector {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const ::foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(::domain::face::detector::DetectionResults, detect, (const cv::Mat&), (override));
};

} // namespace tests::mocks::domain
