#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
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

TEST_F(FaceStoreTest, ConcurrentReadWrite) {
    const int num_threads = 8;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_reads{0};
    std::atomic<int> successful_writes{0};

    // Writer threads
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([this, i, operations_per_thread, &successful_writes]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string name = "thread_" + std::to_string(i) + "_op_" + std::to_string(j);
                store.insert_faces(name, faces1);
                successful_writes++;
            }
        });
    }

    // Reader threads
    for (int i = 0; i < num_threads / 2; ++i) {
        threads.emplace_back([this, operations_per_thread, &successful_reads]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                // Read operations should not crash even with concurrent writes
                [[maybe_unused]] auto result = store.get_faces("thread_0_op_0");
                [[maybe_unused]] bool contains = store.is_contains("thread_0_op_0");
                successful_reads++;
            }
        });
    }

    for (auto& t : threads) { t.join(); }

    EXPECT_EQ(successful_writes.load(), (num_threads / 2) * operations_per_thread);
    EXPECT_EQ(successful_reads.load(), (num_threads / 2) * operations_per_thread);
}

TEST_F(FaceStoreTest, ConcurrentReadOnly) {
    // Pre-populate store
    store.insert_faces("test_key", faces1);
    store.insert_faces(frame1, faces2);

    const int num_threads = 8;
    const int reads_per_thread = 200;
    std::vector<std::thread> threads;
    std::atomic<int> total_reads{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, reads_per_thread, &total_reads]() {
            for (int j = 0; j < reads_per_thread; ++j) {
                auto result1 = store.get_faces("test_key");
                auto result2 = store.get_faces(frame1);
                EXPECT_EQ(result1.size(), 1);
                EXPECT_EQ(result2.size(), 1);
                total_reads += 2;
            }
        });
    }

    for (auto& t : threads) { t.join(); }

    EXPECT_EQ(total_reads.load(), num_threads * reads_per_thread * 2);
}
