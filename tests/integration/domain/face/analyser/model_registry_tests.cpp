/**
 * @file model_registry_tests.cpp
 * @brief Unit tests for FaceModelRegistry.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>

import domain.face.model_registry;
import domain.face.detector;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.logger;

using namespace domain::face;
using namespace domain::face::detector;
using namespace domain::ai::model_repository;
using namespace foundation::ai::inference_session;

class FaceModelRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_repo = ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");
    }
    std::shared_ptr<ModelRepository> model_repo;
};

TEST_F(FaceModelRegistryTest, SingletonInstance) {
    auto instance1 = FaceModelRegistry::get_instance();
    auto instance2 = FaceModelRegistry::get_instance();
    EXPECT_EQ(instance1, instance2);
}

TEST_F(FaceModelRegistryTest, GetDetectorReuse) {
    auto registry = FaceModelRegistry::get_instance();
    registry->clear();

    std::string path = model_repo->ensure_model("scrfd");
    ASSERT_FALSE(path.empty());

    Options opts;

    auto det1 = registry->get_detector(DetectorType::SCRFD, path, opts);
    ASSERT_NE(det1, nullptr);

    auto det2 = registry->get_detector(DetectorType::SCRFD, path, opts);
    EXPECT_EQ(det1, det2); // Should be the same instance

    // Change options (non-device ID to avoid errors on single-GPU systems)
    opts.trt_max_workspace_size = 1;
    auto det3 = registry->get_detector(DetectorType::SCRFD, path, opts);
    EXPECT_NE(det1, det3); // Should be a different instance
}
