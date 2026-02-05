#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <cstdint>

import domain.face;
import domain.face.selector;
import domain.common;

using namespace domain::face;
using namespace domain::face::selector;
using namespace domain::common::types;

namespace {

Face create_face(float x, float y, float w, float h, float score = 0.5f) {
    Face face;
    face.set_box({x, y, w, h});
    face.set_detector_score(score);
    return face;
}

Face create_face_with_attributes(Gender gender, Race race, std::uint16_t age_min, std::uint16_t age_max) {
    Face face;
    face.set_gender(gender);
    face.set_race(race);
    face.set_age_range({age_min, age_max});
    return face;
}

Face create_face_with_embedding(const std::vector<float>& emb) {
    Face face;
    face.set_normed_embedding(emb);
    return face;
}

} // namespace

class FaceSelectorTest : public ::testing::Test {
protected:
    std::vector<Face> faces;

    void SetUp() override {
        // Prepare some common faces
    }
};

// --- Sorting Tests ---

TEST_F(FaceSelectorTest, SortByLeftRight) {
    faces = {
        create_face(100, 0, 50, 50), // Right
        create_face(0, 0, 50, 50),   // Left
        create_face(50, 0, 50, 50)   // Middle
    };
    Options opts;
    opts.order = Order::LeftRight;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].box().x, 0);
    EXPECT_EQ(result[1].box().x, 50);
    EXPECT_EQ(result[2].box().x, 100);
}

TEST_F(FaceSelectorTest, SortByRightLeft) {
    faces = {
        create_face(0, 0, 50, 50),   // Left
        create_face(100, 0, 50, 50)  // Right
    };
    Options opts;
    opts.order = Order::RightLeft;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].box().x, 100);
    EXPECT_EQ(result[1].box().x, 0);
}

TEST_F(FaceSelectorTest, SortByTopBottom) {
    faces = {
        create_face(0, 100, 50, 50), // Bottom
        create_face(0, 0, 50, 50)    // Top
    };
    Options opts;
    opts.order = Order::TopBottom;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].box().y, 0);
    EXPECT_EQ(result[1].box().y, 100);
}

TEST_F(FaceSelectorTest, SortByBottomTop) {
    faces = {
        create_face(0, 0, 50, 50),    // Top
        create_face(0, 100, 50, 50)   // Bottom
    };
    Options opts;
    opts.order = Order::BottomTop;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].box().y, 100);
    EXPECT_EQ(result[1].box().y, 0);
}

TEST_F(FaceSelectorTest, SortBySmallLarge) {
    faces = {
        create_face(0, 0, 100, 100), // Large (10000)
        create_face(0, 0, 10, 10)    // Small (100)
    };
    Options opts;
    opts.order = Order::SmallLarge;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].box().area(), 100);
    EXPECT_EQ(result[1].box().area(), 10000);
}

TEST_F(FaceSelectorTest, SortByLargeSmall) {
    faces = {
        create_face(0, 0, 10, 10),    // Small
        create_face(0, 0, 100, 100)   // Large
    };
    Options opts;
    opts.order = Order::LargeSmall;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].box().area(), 10000);
    EXPECT_EQ(result[1].box().area(), 100);
}

TEST_F(FaceSelectorTest, SortByBestWorst) {
    faces = {
        create_face(0, 0, 50, 50, 0.2f), // Worst
        create_face(0, 0, 50, 50, 0.9f)  // Best
    };
    Options opts;
    opts.order = Order::BestWorst;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_NEAR(result[0].detector_score(), 0.9f, 0.001f);
    EXPECT_NEAR(result[1].detector_score(), 0.2f, 0.001f);
}

TEST_F(FaceSelectorTest, SortByWorstBest) {
    faces = {
        create_face(0, 0, 50, 50, 0.9f), // Best
        create_face(0, 0, 50, 50, 0.2f)  // Worst
    };
    Options opts;
    opts.order = Order::WorstBest;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    EXPECT_NEAR(result[0].detector_score(), 0.2f, 0.001f);
    EXPECT_NEAR(result[1].detector_score(), 0.9f, 0.001f);
}

// --- Filtering Tests ---

TEST_F(FaceSelectorTest, FilterByGender) {
    faces = {
        create_face_with_attributes(Gender::Male, Race::White, 25, 30),
        create_face_with_attributes(Gender::Female, Race::White, 25, 30)
    };
    Options opts;
    opts.genders = {Gender::Female};

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].gender(), Gender::Female);
}

TEST_F(FaceSelectorTest, FilterByRace) {
    faces = {
        create_face_with_attributes(Gender::Male, Race::White, 25, 30),
        create_face_with_attributes(Gender::Male, Race::Black, 25, 30),
        create_face_with_attributes(Gender::Male, Race::Asian, 25, 30)
    };
    Options opts;
    opts.races = {Race::Asian, Race::Black};

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 2);
    for (const auto& face : result) {
        EXPECT_TRUE(face.race() == Race::Asian || face.race() == Race::Black);
    }
}

TEST_F(FaceSelectorTest, FilterByAge) {
    faces = {
        create_face_with_attributes(Gender::Male, Race::White, 10, 15), // Kid
        create_face_with_attributes(Gender::Male, Race::White, 25, 30), // Adult
        create_face_with_attributes(Gender::Male, Race::White, 60, 70)  // Senior
    };
    Options opts;
    opts.age_start = 20;
    opts.age_end = 50;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].age_range().min, 25u);
}

// --- Similarity Tests ---

TEST_F(FaceSelectorTest, FilterBySimilarity) {
    // Orthogonal vectors: similarity 0
    // Same vectors: similarity 1
    std::vector<float> emb1 = {1.0f, 0.0f};
    std::vector<float> emb2 = {0.0f, 1.0f};
    std::vector<float> emb3 = {1.0f, 0.0f}; // Match emb1

    faces = {
        create_face_with_embedding(emb2),
        create_face_with_embedding(emb3)
    };

    Options opts;
    opts.mode = SelectorMode::Reference;
    opts.reference_face = create_face_with_embedding(emb1);
    opts.similarity_threshold = 0.9f;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 1);
    // Should match emb3
    EXPECT_EQ(result[0].normed_embedding(), emb3);
}

TEST_F(FaceSelectorTest, FilterBySimilarityNoReference) {
    faces = { create_face_with_embedding({1.0f, 0.0f}) };
    Options opts;
    opts.mode = SelectorMode::Reference;
    opts.reference_face = std::nullopt; // No reference

    auto result = select_faces(faces, opts);

    // Should return all faces if no reference is provided (based on logic in cpp: "if (!opts.reference_face.has_value() ... return faces;")
    EXPECT_EQ(result.size(), 1);
}

// --- Mode Tests ---

TEST_F(FaceSelectorTest, ModeOneReturnsFirstAfterSort) {
    faces = {
        create_face(100, 0, 50, 50), // Right
        create_face(0, 0, 50, 50)    // Left
    };
    Options opts;
    opts.mode = SelectorMode::One;
    opts.order = Order::LeftRight; // Should sort Left first

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].box().x, 0); // The one on the left
}

TEST_F(FaceSelectorTest, ModeManyReturnsAllAfterFilter) {
    faces = {
        create_face(100, 0, 50, 50),
        create_face(0, 0, 50, 50)
    };
    Options opts;
    opts.mode = SelectorMode::Many;

    auto result = select_faces(faces, opts);

    EXPECT_EQ(result.size(), 2);
}

TEST_F(FaceSelectorTest, EmptyInputReturnsEmpty) {
    faces = {};
    Options opts;
    auto result = select_faces(faces, opts);
    EXPECT_TRUE(result.empty());
}
