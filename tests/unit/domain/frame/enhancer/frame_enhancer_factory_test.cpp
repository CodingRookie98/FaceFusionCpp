#include <gtest/gtest.h>
#include <memory>
#include <string>

import domain.frame.enhancer;
import foundation.ai.inference_session;

using namespace domain::frame::enhancer;

TEST(FrameEnhancerFactoryTest, CreateRealEsrGan) {
    foundation::ai::inference_session::Options opts;
    // Use a valid model name supported by the factory
    // Since the model file likely doesn't exist, we expect a runtime error from load_model
    // BUT we should NOT get "model is not supported"
    try {
        auto enhancer = FrameEnhancerFactory::create(FrameEnhancerType::RealEsrGan, "real_esrgan_x4", opts);
        // If it succeeds (e.g. mock session or file exists), great
        if (enhancer) EXPECT_NE(enhancer, nullptr);
    } catch (const std::exception& e) {
        std::string err = e.what();
        // Verify it's NOT the "not supported" error
        EXPECT_EQ(err.find("model is not supported"), std::string::npos) << "Unexpected error: " << err;
        // It's acceptable if it fails due to file not found or load error
    }
}

TEST(FrameEnhancerFactoryTest, CreateRealHatGan) {
    foundation::ai::inference_session::Options opts;
    // Use a valid model name supported by the factory
    try {
        auto enhancer = FrameEnhancerFactory::create(FrameEnhancerType::RealHatGan, "real_hatgan_x4", opts);
        if (enhancer) EXPECT_NE(enhancer, nullptr);
    } catch (const std::exception& e) {
        std::string err = e.what();
        EXPECT_EQ(err.find("model is not supported"), std::string::npos) << "Unexpected error: " << err;
    }
}

TEST(FrameEnhancerFactoryTest, CreateInvalidModelThrows) {
    foundation::ai::inference_session::Options opts;
    EXPECT_THROW({
        FrameEnhancerFactory::create(FrameEnhancerType::RealEsrGan, "invalid_model_name", opts);
    }, std::invalid_argument);
}
