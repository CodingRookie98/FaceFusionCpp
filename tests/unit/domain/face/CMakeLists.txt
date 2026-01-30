add_facefusion_test(
    domain_face_tests
    SOURCES
        face_store_test.cpp

    LINK_LIBRARIES
        domain_face
        domain_face_test_support
        foundation_infrastructure_test_support
)

add_subdirectory(analyser)

