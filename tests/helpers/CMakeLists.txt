add_library(test_helpers STATIC)

target_sources(test_helpers
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            domain/face_test_helpers.ixx
            foundation/test_utilities.ixx
            foundation/memory_monitor.ixx
    PRIVATE
        environment/global_test_environment.cpp
)

target_link_libraries(test_helpers
    PUBLIC
        test_common
        domain_face
        domain_face_analyser
        foundation_ai
        foundation_infrastructure
        ${OpenCV_LIBS}
)

if(CUDA_SYNC_AVAILABLE)
    target_compile_definitions(test_helpers PRIVATE CUDA_SYNC_AVAILABLE)
    target_link_libraries(test_helpers PRIVATE CUDA::cudart)
endif()

target_compile_features(test_helpers PUBLIC cxx_std_20)

target_include_directories(test_helpers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
