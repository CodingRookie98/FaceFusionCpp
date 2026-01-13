add_executable(test_domain_face_detector test_face_detector.cpp)

target_link_libraries(test_domain_face_detector
    PRIVATE
        domain_face_detector
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME test_domain_face_detector COMMAND test_domain_face_detector)
