module;
#include <gmock/gmock.h>
#include <string>
#include <opencv2/core.hpp>

export module tests.mocks.domain.mock_face_enhancer;

import domain.face.enhancer;
import foundation.ai.inference_session;

export namespace tests::mocks::domain {

class MockFaceEnhancer : public ::domain::face::enhancer::IFaceEnhancer {
public:
    MOCK_METHOD(void, load_model, (const std::string&, const ::foundation::ai::inference_session::Options&), (override));
    MOCK_METHOD(cv::Mat, enhance_face, (const cv::Mat&), (override));
    MOCK_METHOD(cv::Size, get_model_input_size, (), (const, override));
};

}
