#include <gtest/gtest.h>
#include <memory>
#include <string>

import domain.face.masker;
import foundation.ai.inference_session;

using namespace domain::face::masker;

TEST(FaceMaskerFactoryTest, CreateOcclusionMasker) {
    // The factory or impl checks for file existence.
    // We expect it to throw runtime_error for dummy path.
    EXPECT_THROW({
        auto masker = create_occlusion_masker("dummy_path");
    }, std::runtime_error);
}

TEST(FaceMaskerFactoryTest, CreateRegionMasker) {
    // The factory or impl checks for file existence.
    // We expect it to throw runtime_error for dummy path.
    EXPECT_THROW({
        auto masker = create_region_masker("dummy_path");
    }, std::runtime_error);
}
