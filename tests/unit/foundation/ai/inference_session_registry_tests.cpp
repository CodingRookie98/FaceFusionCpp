#include <gtest/gtest.h>
#include <memory>
#include <string>

import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

using namespace foundation::ai::inference_session;

TEST(InferenceSessionRegistryTest, SingletonInstance) {
    auto& registry1 = InferenceSessionRegistry::get_instance();
    auto& registry2 = InferenceSessionRegistry::get_instance();
    EXPECT_EQ(&registry1, &registry2);
}

TEST(InferenceSessionRegistryTest, SharingSession) {
    // We can't easily test actual model loading without a real model file,
    // but we can test the registry logic by mock or just checking null for empty path
    auto& registry = InferenceSessionRegistry::get_instance();

    // Empty path should return nullptr
    auto session1 = registry.get_session("", Options{});
    EXPECT_EQ(session1, nullptr);

    // Since we don't have a valid model file here,
    // real get_session calls will throw or return nullptr if file not found.
    // But we can verify that for the same path (even if it doesn't exist yet),
    // the registry logic would attempt to share or create.

    // Actually, InferenceSession::load_model throws if file doesn't exist.
    // So we can't easily test the success path without a temp file.
}
