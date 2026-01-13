add_facefusion_test(
    domain_face_tests
    SOURCES
        test_face.cpp
        face_helper_test.cpp
        face_selector_test.cpp
        face_store_test.cpp
    LINK_LIBRARIES
        domain_face
)

add_subdirectory(detector)
