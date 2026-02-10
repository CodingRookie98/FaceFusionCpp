module;
#include <vector>
#include <opencv2/core.hpp>

export module tests.common.generators.face_generator;

import domain.face;

export namespace tests::common::generators {

    domain::face::Face create_valid_face() {
        domain::face::Face face;
        // Create 5 landmarks
        std::vector<cv::Point2f> kps(5);
        face.set_kps(kps);
        
        // Create embedding
        std::vector<float> embedding(512, 0.5f);
        face.set_embedding(embedding);
        
        return face;
    }

}
