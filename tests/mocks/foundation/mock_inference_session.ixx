module;
#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <onnxruntime_cxx_api.h>

export module tests.mocks.foundation.mock_inference_session;

import foundation.ai.inference_session;

export namespace tests::mocks::foundation {

class MockInferenceSession : public ::foundation::ai::inference_session::InferenceSession {
public:
    MOCK_METHOD(void, load_model,
                (const std::string&, const ::foundation::ai::inference_session::Options&),
                (override));
    MOCK_METHOD(bool, is_model_loaded, (), (const, override));
    MOCK_METHOD(std::string, get_loaded_model_path, (), (const, override));
    MOCK_METHOD(std::vector<Ort::Value>, run, (const std::vector<Ort::Value>&), (override));
    MOCK_METHOD(std::vector<std::vector<int64_t>>, get_input_node_dims, (), (const, override));
    MOCK_METHOD(std::vector<std::vector<int64_t>>, get_output_node_dims, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, get_input_names, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, get_output_names, (), (const, override));
};

} // namespace tests::mocks::foundation
