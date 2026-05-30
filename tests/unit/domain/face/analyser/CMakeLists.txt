add_facefusion_test(
    face_analyser_unit_tests
    MAIN_LIB GTest::gmock_main
    SOURCES
        face_analyser_unit_tests.cpp
    LINK_LIBRARIES
        domain_face_analyser
        domain_face_detector
        domain_face_recognizer
        domain_face_classifier
        domain_face
        domain_ai
)
