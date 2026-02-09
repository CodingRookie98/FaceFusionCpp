add_subdirectory(domain/face)
add_subdirectory(foundation/infrastructure)
add_subdirectory(foundation/ai)

# 定义集成测试支持库
add_library(integration_test_support STATIC
    src/integration/global_test_environment.cpp
)

target_link_libraries(integration_test_support
    PUBLIC
        GTest::gtest
        domain_face
        domain_face_analyser
        foundation_ai
)

target_include_directories(integration_test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# CUDA 同步支持 (用于 TearDown 时等待 GPU 操作完成)
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    target_compile_definitions(integration_test_support PRIVATE CUDA_SYNC_AVAILABLE)
    target_link_libraries(integration_test_support PRIVATE CUDA::cudart)
    message(STATUS "integration_test_support: CUDA sync enabled")
endif()

