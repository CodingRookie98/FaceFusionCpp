/**
 * @file face_selector_test.cpp
 * @brief Unit tests for face selection logic.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <vector>
#include <unordered_set>

import domain.face;
import domain.face.selector;
import domain.common;

using namespace domain::face;
using namespace domain::face::selector;
using namespace domain::common::types;

extern void LinkGlobalTestEnvironment();

class FaceSelectorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        // Create a set of diverse faces

        // Face 1: Young Male White, Left side, Low score
        Face f1;
        f1.set_box({0, 0, 50, 50});
        f1.set_detector_score(0.5f);
        f1.set_gender(Gender::Male);
        f1.set_race(Race::White);
        f1.set_age_range({20, 25});
        faces.push_back(f1);

        // Face 2: Old Female Asian, Right side, High score
        Face f2;
        f2.set_box({100, 0, 60, 60}); // Larger area
        f2.set_detector_score(0.9f);
        f2.set_gender(Gender::Female);
        f2.set_race(Race::Asian);
        f2.set_age_range({60, 70});
        faces.push_back(f2);

        // Face 3: Child Male Black, Middle, Medium score
        Face f3;
        f3.set_box({50, 0, 40, 40}); // Smallest area
        f3.set_detector_score(0.7f);
        f3.set_gender(Gender::Male);
        f3.set_race(Race::Black);
        f3.set_age_range({5, 10});
        faces.push_back(f3);
    }

    std::vector<Face> faces;
};

TEST_F(FaceSelectorTest, FilterByGender) {
    Options opts;
    opts.genders = {Gender::Female};

    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].gender(), Gender::Female);
}

TEST_F(FaceSelectorTest, FilterByRace) {
    Options opts;
    opts.races = {Race::Black, Race::Asian};

    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 2);
    // Logic preserves relative order if not sorted, but select applies sort.
    // Default sort is LeftRight.
    // f3 (Black) is at x=50, f2 (Asian) is at x=100.
    EXPECT_EQ(result[0].race(), Race::Black);
    EXPECT_EQ(result[1].race(), Race::Asian);
}

TEST_F(FaceSelectorTest, FilterByAge) {
    Options opts;
    opts.age_start = 10;
    opts.age_end = 30;

    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].race(), Race::White); // f1
}

TEST_F(FaceSelectorTest, SortByPosition) {
    Options opts;
    opts.order = Order::LeftRight; // x ascending
    // f1(0), f3(50), f2(100)
    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].box().x, 0);
    EXPECT_EQ(result[1].box().x, 50);
    EXPECT_EQ(result[2].box().x, 100);

    opts.order = Order::RightLeft; // x descending
    result = select_faces(faces, opts);
    EXPECT_EQ(result[0].box().x, 100);
    EXPECT_EQ(result[1].box().x, 50);
    EXPECT_EQ(result[2].box().x, 0);
}

TEST_F(FaceSelectorTest, SortByScore) {
    Options opts;
    opts.order = Order::BestWorst; // Score desc
    // f2(0.9), f3(0.7), f1(0.5)
    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 3);
    EXPECT_FLOAT_EQ(result[0].detector_score(), 0.9f);
    EXPECT_FLOAT_EQ(result[1].detector_score(), 0.7f);
    EXPECT_FLOAT_EQ(result[2].detector_score(), 0.5f);
}

TEST_F(FaceSelectorTest, SortByArea) {
    Options opts;
    opts.order = Order::SmallLarge; // Area asc
    // f3(40x40=1600), f1(50x50=2500), f2(60x60=3600)
    auto result = select_faces(faces, opts);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].gender(), Gender::Male); // f3
    EXPECT_EQ(result[0].race(), Race::Black);    // f3

    EXPECT_EQ(result[2].gender(), Gender::Female); // f2
}
