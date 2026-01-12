add_executable(domain_face_tests
    test_face.cpp
    face_helper_test.cpp
    face_selector_test.cpp
    face_store_test.cpp
)

target_link_libraries(domain_face_tests
    PRIVATE
        domain_face
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(domain_face_tests)
