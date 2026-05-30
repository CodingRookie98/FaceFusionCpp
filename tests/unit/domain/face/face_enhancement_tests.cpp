#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <optional>
#include <cmath>

import domain.face;
import domain.face.selector;
import domain.face.helper;

#include "face_generated.h"

using namespace domain::face;
using namespace domain::face::selector;
using namespace domain::face::helper;

TEST(FaceEnhancementTest, MaskDeepCopy) {
    Face face1;
    cv::Mat mask = cv::Mat::ones(100, 100, CV_8UC1) * 255;
    face1.set_mask(mask);

    EXPECT_FALSE(face1.mask().empty());

    // Test Copy Constructor
    Face face2(face1);
    EXPECT_FALSE(face2.mask().empty());
    EXPECT_EQ(face1.mask().size(), face2.mask().size());
    // Ensure deep copy (data pointers different)
    if (!face1.mask().empty() && !face2.mask().empty()) {
        EXPECT_NE(face1.mask().data, face2.mask().data);
    }

    // Test Assignment
    Face face3;
    face3 = face1;
    EXPECT_FALSE(face3.mask().empty());
    if (!face1.mask().empty() && !face3.mask().empty()) {
        EXPECT_NE(face1.mask().data, face3.mask().data);
    }
}

TEST(FaceEnhancementTest, SelectorReferenceMode) {
    Face ref_face;
    ref_face.set_normed_embedding({1.0f, 0.0f, 0.0f}); // Unit vector

    Face face_similar;
    face_similar.set_normed_embedding({0.99f, 0.1f, 0.0f}); // Very similar

    Face face_diff;
    face_diff.set_normed_embedding({0.0f, 1.0f, 0.0f}); // Orthogonal

    std::vector<Face> faces = {face_similar, face_diff};

    Options opts;
    opts.mode = SelectorMode::Reference;
    opts.reference_face = ref_face;
    opts.similarity_threshold = 0.5f;

    auto result = select_faces(faces, opts);

    ASSERT_EQ(result.size(), 1);
    // face_similar should match.

    opts.similarity_threshold = 0.999f; // Too high
    result = select_faces(faces, opts);
    EXPECT_EQ(result.size(), 0);
}

TEST(FaceEnhancementTest, AverageEmbedding) {
    Face f1;
    f1.set_embedding({1.0f, 1.0f});
    Face f2;
    f2.set_embedding({3.0f, 3.0f});

    std::vector<Face> faces = {f1, f2};
    auto avg = compute_average_embedding(faces);

    // Avg should be {2, 2}, normalized -> {1/sqrt(2), 1/sqrt(2)} = {0.707, 0.707}
    EXPECT_NEAR(avg[0], 0.70710678f, 1e-4);
    EXPECT_NEAR(avg[1], 0.70710678f, 1e-4);

    // Check norm
    float norm = std::sqrt(avg[0] * avg[0] + avg[1] * avg[1]);
    EXPECT_NEAR(norm, 1.0f, 1e-4);
}

TEST(FaceEnhancementTest, FlatBuffersSchema) {
    flatbuffers::FlatBufferBuilder builder(1024);

    auto box = domain::face::serialization::Rect(0, 0, 100, 100);

    std::vector<float> emb = {0.1f, 0.2f};
    auto emb_offset = builder.CreateVector(emb);

    auto face_buffer = domain::face::serialization::CreateFaceBuffer(builder, &box,
                                                                     0, // landmarks
                                                                     emb_offset
                                                                     // ... other fields default
    );

    std::vector<flatbuffers::Offset<domain::face::serialization::FaceBuffer>> faces_vector;
    faces_vector.push_back(face_buffer);

    auto faces_offset = builder.CreateVector(faces_vector);

    auto channel = domain::face::serialization::CreateFaceListChannel(builder, faces_offset);

    builder.Finish(channel);

    uint8_t* buf = builder.GetBufferPointer();
    auto root = domain::face::serialization::GetFaceListChannel(buf);

    ASSERT_EQ(root->faces()->size(), 1);
    auto f = root->faces()->Get(0);
    EXPECT_NEAR(f->box()->width(), 100.0f, 1e-5);
    EXPECT_NEAR(f->embedding()->Get(0), 0.1f, 1e-5);
}
