add_facefusion_test(
    domain_face_tests
    SOURCES
        test_face.cpp
        face_helper_test.cpp
        face_selector_test.cpp
        face_store_test.cpp

    LINK_LIBRARIES
        domain_face
        domain_face_test_support
        foundation_infrastructure_test_support
)

add_subdirectory(detector)
add_subdirectory(swapper)
add_subdirectory(classifier)
add_subdirectory(analyser)
add_subdirectory(enhancer)
add_subdirectory(expression)
add_subdirectory(landmarker)
add_subdirectory(masker)
add_subdirectory(recognizer)
