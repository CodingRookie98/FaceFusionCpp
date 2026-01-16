
add_facefusion_test(domain_face_enhancer_tests
    SOURCES
    face_enhancer_tests.cpp
    LINK_LIBRARIES
    domain_face
    domain_face_detector
    foundation_ai
    GTest::gtest
    GTest::gtest_main
    ${OpenCV_LIBS}
)
