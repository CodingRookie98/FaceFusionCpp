#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.classifier;
import domain.face;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::face::classifier;
using namespace domain::face;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class FairFaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance().clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(FairFaceTest, LoadModelAndClassify) {
    std::string model_path = "fairface.onnx";
    InferenceSessionRegistry::get_instance().preload_session(model_path, Options(), mock_session);

    auto classifier = create_classifier(ClassifierType::FairFace);

    // 1. Setup Mock for load_model
    // Input Dims: [1, 3, 224, 224]
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 224, 224}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));
    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));

    Options options;
    classifier->load_model(model_path, options);

    // 2. Setup Mock for run
    // The code expects 3 outputs: [Race(0-6), Gender(0-1), Age(0-8)]
    // Each output is a tensor of int64_t, shape [1] ?
    // Let's check impl:
    // int64_t raceId = outputTensor[0].GetTensorData<int64_t>()[0];
    // int64_t genderId = outputTensor[1].GetTensorData<int64_t>()[0];
    // int64_t ageId = outputTensor[2].GetTensorData<int64_t>()[0];

    int64_t race_val = 3;   // Asian
    int64_t gender_val = 1; // Female
    int64_t age_val = 3;    // 20-29

    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([&](const std::vector<Ort::Value>&) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;

        // Race
        std::vector<int64_t> shape = {1};
        outputs.push_back(
            Ort::Value::CreateTensor<int64_t>(memory_info, const_cast<int64_t*>(&race_val), 1,
                                              const_cast<int64_t*>(shape.data()), shape.size()));

        // Gender
        outputs.push_back(
            Ort::Value::CreateTensor<int64_t>(memory_info, const_cast<int64_t*>(&gender_val), 1,
                                              const_cast<int64_t*>(shape.data()), shape.size()));

        // Age
        outputs.push_back(
            Ort::Value::CreateTensor<int64_t>(memory_info, const_cast<int64_t*>(&age_val), 1,
                                              const_cast<int64_t*>(shape.data()), shape.size()));

        return outputs;
    });

    // 3. Execute
    cv::Mat image = cv::Mat::zeros(512, 512, CV_8UC3);
    domain::face::types::Landmarks landmarks = {
        {200, 200}, {300, 200}, {250, 250}, {220, 300}, {280, 300}};

    // In FairFace implementation:
    // outputTensor[0] -> Race
    // outputTensor[1] -> Gender
    // outputTensor[2] -> Age

    // BUT we mocked the return order in step 2:
    // 1. Race
    // 2. Gender
    // 3. Age

    // Let's check the test failure again:
    // Expected: Asian (3)
    // Actual: 5 (Indian?)

    // Wait, the failure says:
    // Expected: Asian (3)
    // Actual: 5

    // Let's check categorizeRace:
    // if (raceId == 3 || raceId == 4) return Asian;
    // if (raceId == 5) return Indian;

    // So the mock returned 5?
    // In test: int64_t race_val = 3;

    // Ah! I might have mixed up the outputs or indices.
    // The implementation:
    // int64_t raceId = outputTensor[0].GetTensorData<int64_t>()[0];
    // int64_t genderId = outputTensor[1].GetTensorData<int64_t>()[0];
    // int64_t ageId = outputTensor[2].GetTensorData<int64_t>()[0];

    // The Mock:
    // outputs.push_back(race)
    // outputs.push_back(gender)
    // outputs.push_back(age)

    // Wait, why did it get 5?
    // Maybe the model outputs are NOT in that order?
    // FairFace implementation assumes indices 0, 1, 2 correspond to Race, Gender, Age.
    // However, ONNX Runtime usually returns outputs based on the order of output nodes in the
    // model. In the real model, the order might be fixed. In my Mock, I return a vector.
    // `session->run` returns a vector.

    // If I passed `race_val = 3` and got 5, where did 5 come from?
    // I didn't set any 5 in the test code shown above.
    // Let's look at the test code again.
    // int64_t race_val = 3;
    // int64_t gender_val = 1;
    // int64_t age_val = 3;

    // Is it possible that `GetTensorData` is reading garbage?
    // I am passing `&race_val` to `CreateTensor`.
    // `race_val` is a local variable in the test function scope.
    // The lambda captures `[=]`. So it captures a COPY of `race_val` by value?
    // No, `int64_t race_val = 3;` is a local var.
    // Lambda `[=]` captures `race_val` by value (so it has its own copy).
    // But `&race_val` inside lambda would take address of the CAPTURED variable.
    // `Ort::Value::CreateTensor` takes a pointer to the data buffer.
    // Does `CreateTensor` COPY the data or just hold the pointer?
    // The overload `CreateTensor<T>(info, T* p_data, size_t p_data_element_count, ...)`
    // "This API does not copy the data. It wraps the data..."
    // So it uses the pointer `p_data` directly!
    // Inside the lambda: `const_cast<int64_t*>(&race_val)` takes the address of the lambda's member
    // variable (the captured copy). The lambda returns `outputs` vector containing `Ort::Value`s
    // which point to the lambda's member variables. When `run` returns, the lambda is destroyed?
    // `WillRepeatedly` stores the action (the lambda). The lambda itself persists as long as the
    // expectation exists. So the lambda object is alive. BUT `outputs` vector is returned by value.
    // The `Ort::Value`s are moved/copied. The `Ort::Value` holds the pointer to the data. The data
    // is inside the lambda object. So as long as the lambda object is alive, the pointer should be
    // valid? Yes, Google Mock actions are kept alive.

    // However, `[=]` captures by value. The closure object has a member `race_val`.
    // When the lambda is called, `&race_val` is the address of that member.
    // That seems correct.

    // Wait, `outputs` is a local vector in the lambda.
    // We return `outputs`.
    // The `Ort::Value`s inside are returned.
    // They point to `&race_val` which is inside the lambda.
    // This *should* be fine if the lambda instance used for the call is stable.

    // Let's check why we got 5.
    // Maybe `race_val` isn't 3?
    // I set `int64_t race_val = 3;`
    // Wait, failure said:
    // Expected: Asian (3)
    // Actual: 5

    // Where does 5 come from?
    // 5 is Indian.
    // Maybe the implementation logic is different?
    // FairFace.cpp:
    // if (raceId == 5) return Indian;

    // So the implementation received 5.
    // Why did it receive 5?

    // Maybe I am misinterpreting the error message.
    // Expected equality of these values:
    // result.race
    //   Which is: 1-byte object <05>
    // domain::common::types::Race::Asian
    //   Which is: 1-byte object <03>

    // So result.race IS 5 (Indian).
    // Expected is 3 (Asian).

    // So the code got 5 from the tensor.
    // But I put 3 in the tensor.

    // Is it possible that the memory is corrupted?
    // Using `CreateTensor` with user-owned buffer is risky if lifetime is tricky.
    // Let's use `CreateTensor` that copies data?
    // There isn't one that easily copies from a pointer without owning it unless we use a vector
    // and keep it alive.

    // Safer approach:
    // Make the data static or member of the test fixture.
    // Or capture by reference `[&]` but make sure the variables are alive?
    // `race_val` is local to `TEST_F`.
    // `EXPECT_CALL` is in `TEST_F`.
    // The lambda is executed inside `classifier->classify`.
    // `classifier->classify` is called inside `TEST_F`.
    // So `race_val` is definitely alive.
    // BUT `[=]` captures by COPY.
    // So the lambda has its own copy.

    // Let's try to make the data static in the lambda to be safe?
    // Or change capture to `[&]`?
    // `[&]` captures `race_val` by reference.
    // `race_val` is on the stack of `TEST_F`.
    // The lambda is called while `TEST_F` is running `classify`.
    // So the stack frame is valid.
    // `&race_val` will be the address on the stack.

    // Let's try changing `[=]` to `[&]`.

    // Wait, I see `static float scalar_val` in previous test (LivePortrait).
    // Maybe that's why it worked there.

    // Let's use `static` variables inside the lambda or capture by reference.
    // Capture by reference is safer here because the test function outlives the call.

    auto result = classifier->classify(image, landmarks);

    // 4. Verify
    EXPECT_EQ(result.race, domain::common::types::Race::Asian);
    EXPECT_EQ(result.gender, domain::common::types::Gender::Female);
    EXPECT_EQ(result.age.min, 20);
    EXPECT_EQ(result.age.max, 29);
}
