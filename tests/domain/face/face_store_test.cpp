#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

import domain.face;
import domain.face.store;

using namespace domain::face;
using namespace domain::face::store;

class FaceStoreTest : public ::testing::Test {
protected:
    FaceStore store;
    cv::Mat frame1;
    cv::Mat frame2;
    std::vector<Face> faces1;
    std::vector<Face> faces2;

    void SetUp() override {
        // Create dummy frames
        frame1 = cv::Mat::zeros(100, 100, CV_8UC3);
        frame2 = cv::Mat::ones(100, 100, CV_8UC3);

        // Create dummy faces
        Face f1;
        f1.set_detector_score(0.9f);
        faces1.push_back(f1);

        Face f2;
        f2.set_detector_score(0.8f);
        faces2.push_back(f2);
    }
};

TEST_F(FaceStoreTest, FrameHashConsistency) {
    std::string hash1 = FaceStore::create_frame_hash(frame1);
    std::string hash1_again = FaceStore::create_frame_hash(frame1);
    EXPECT_EQ(hash1, hash1_again);

    std::string hash2 = FaceStore::create_frame_hash(frame2);
    EXPECT_NE(hash1, hash2);
}

TEST_F(FaceStoreTest, InsertAndGetByFrame) {
    store.insert_faces(frame1, faces1);

    EXPECT_TRUE(store.is_contains(frame1));

    auto retrieved = store.get_faces(frame1);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0].detector_score(), 0.9f);

    // Non-existent frame
    EXPECT_FALSE(store.is_contains(frame2));
    EXPECT_TRUE(store.get_faces(frame2).empty());
}

TEST_F(FaceStoreTest, InsertAndGetByName) {
    std::string name = "test_group";
    store.insert_faces(name, faces2);

    EXPECT_TRUE(store.is_contains(name));

    auto retrieved = store.get_faces(name);
    ASSERT_EQ(retrieved.size(), 1);
    EXPECT_FLOAT_EQ(retrieved[0].detector_score(), 0.8f);

    EXPECT_FALSE(store.is_contains("non_existent"));
}

TEST_F(FaceStoreTest, RemoveByFrame) {
    store.insert_faces(frame1, faces1);
    EXPECT_TRUE(store.is_contains(frame1));

    store.remove_faces(frame1);
    EXPECT_FALSE(store.is_contains(frame1));
}

TEST_F(FaceStoreTest, RemoveByName) {
    std::string name = "test_group";
    store.insert_faces(name, faces2);
    EXPECT_TRUE(store.is_contains(name));

    store.remove_faces(name);
    EXPECT_FALSE(store.is_contains(name));
}

TEST_F(FaceStoreTest, ClearFaces) {
    store.insert_faces(frame1, faces1);
    store.insert_faces("group", faces2);

    EXPECT_TRUE(store.is_contains(frame1));
    EXPECT_TRUE(store.is_contains("group"));

    store.clear_faces();

    EXPECT_FALSE(store.is_contains(frame1));
    EXPECT_FALSE(store.is_contains("group"));
}

TEST_F(FaceStoreTest, EmptyInsert) {
    std::vector<Face> empty_faces;
    store.insert_faces(frame1, empty_faces);

    // Implementation says: if faces.empty() return;
    // So map should remain empty/unchanged
    EXPECT_FALSE(store.is_contains(frame1));
}
