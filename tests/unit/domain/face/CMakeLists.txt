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
add_subdirectory(swapper)
add_subdirectory(classifier)
add_subdirectory(analyser)
add_subdirectory(enhancer)
add_subdirectory(landmarker)
add_subdirectory(masker)
add_subdirectory(recognizer)
