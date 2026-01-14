add_facefusion_test(
    domain_face_tests
    SOURCES
        test_face.cpp
        face_helper_test.cpp
        face_selector_test.cpp
        face_store_test.cpp
        face_landmarker_tests.cpp

    LINK_LIBRARIES
        domain_face
        domain_face_detector

)

add_subdirectory(detector)
add_subdirectory(classifier)
