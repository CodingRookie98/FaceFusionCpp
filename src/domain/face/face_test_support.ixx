module;
#include <opencv2/opencv.hpp>
#include <vector>

export module domain.face.test_support;

export import domain.face;

export namespace domain::face::test_support {

    Face create_empty_face() {
        return Face();
    }

    Face create_test_face() {
        Face face;
        face.set_box({10.0f, 10.0f, 100.0f, 100.0f});

        types::Landmarks kps;
        for(int i=0; i<5; ++i) {
            kps.emplace_back(static_cast<float>(10.0f + i*10), static_cast<float>(10.0f + i*10));
        }
        face.set_kps(std::move(kps));

        face.set_detector_score(0.95f);
        face.set_landmarker_score(0.98f);

        return face;
    }

    Face create_face_with_68_kps() {
        Face face;
        face.set_box({0.0f, 0.0f, 200.0f, 200.0f});

        types::Landmarks kps;
        kps.reserve(68);
        for(int i=0; i<68; ++i) {
            kps.emplace_back(static_cast<float>(i), static_cast<float>(i));
        }
        face.set_kps(std::move(kps));

        return face;
    }
}
