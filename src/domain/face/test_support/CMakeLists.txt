add_modules_library(domain_face_test_support)

target_sources(domain_face_test_support
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_test_support.ixx
)

target_link_libraries(domain_face_test_support
    PUBLIC
        domain_face
        domain_face_detector
        domain_face_recognizer
        domain_ai
        foundation_ai
)
