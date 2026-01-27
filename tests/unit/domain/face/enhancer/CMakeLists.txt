
add_facefusion_test(domain_face_enhancer_tests
    SOURCES
    face_enhancer_tests.cpp
    LINK_LIBRARIES
    domain_face
    domain_face_detector
    domain_face_test_support
    foundation_ai
    foundation_infrastructure_test_support
    GTest::gtest
    GTest::gtest_main
    ${OpenCV_LIBS}
)
