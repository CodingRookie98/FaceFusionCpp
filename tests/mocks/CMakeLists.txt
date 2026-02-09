add_library(test_mocks STATIC)

target_sources(test_mocks
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            foundation/mock_inference_session.ixx
            domain/mock_face_detector.ixx
            domain/mock_face_enhancer.ixx
            domain/mock_model_repository.ixx
)

target_link_libraries(test_mocks
    PUBLIC
        test_common
        foundation_ai
        domain_face_detector
        domain_face_enhancer
        domain_ai
)

target_compile_features(test_mocks PUBLIC cxx_std_20)

target_include_directories(test_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
