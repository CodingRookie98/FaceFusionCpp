cmake_minimum_required(VERSION 3.30)

# 测试目录的 CMakeLists.txt
# 此文件用于管理所有测试目标

# 包含 GoogleTest 模块
include(GoogleTest)

find_package(ONNX CONFIG REQUIRED)

# Add NVML support
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(NVML_LIBRARY NAMES nvidia-ml libnvidia-ml.so.1 PATHS /usr/lib/x86_64-linux-gnu /usr/lib/wsl/lib /usr/local/cuda/lib64/stubs)
    find_path(NVML_INCLUDE_DIR nvml.h PATHS /usr/local/cuda/include /usr/include /usr/local/cuda-12.8/targets/x86_64-linux/include)
    
    if(NVML_LIBRARY AND NVML_INCLUDE_DIR)
        message(STATUS "Found NVML: ${NVML_LIBRARY}")
        message(STATUS "Found NVML Header: ${NVML_INCLUDE_DIR}")
        set(HAVE_NVML TRUE)
        include_directories(${NVML_INCLUDE_DIR})
    else()
        message(WARNING "NVML not found (Lib: ${NVML_LIBRARY}, Inc: ${NVML_INCLUDE_DIR}), GPU memory monitoring will be disabled")
        set(HAVE_NVML FALSE)
    endif()
endif()

# 添加 ASan 构建选项
option(ENABLE_ASAN "Enable AddressSanitizer for memory leak detection" OFF)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    
    if(MSVC)
        # MSVC ASan
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        # GCC/Clang ASan
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# 辅助函数：创建测试可执行文件
# 参数：
#   TEST_NAME: 测试名称
#   SOURCES: 测试源文件列表
#   LINK_LIBRARIES: 需要链接的库（可选）
function(add_facefusion_test TEST_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        "MAIN_LIB"
        "SOURCES;LINK_LIBRARIES"
    )

    # 创建测试可执行文件
    add_executable(${TEST_NAME} ${ARG_SOURCES})

    # 链接 GoogleTest 主函数
    if(NOT ARG_MAIN_LIB)
        set(ARG_MAIN_LIB GTest::gtest_main)
    endif()
    target_link_libraries(${TEST_NAME} PRIVATE ${ARG_MAIN_LIB})

    # 链接额外的库
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${TEST_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # 自动添加资源复制依赖 (如果资源机制已初始化)
    if(COMMAND setup_resource_copying)
        setup_resource_copying(${TEST_NAME})
    endif()

    # Determine label based on directory
    set(TEST_LABELS "facefusion")
    string(REPLACE "\\" "/" NORMALIZED_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "Configuring test ${TEST_NAME} in ${NORMALIZED_SOURCE_DIR}")

    if(NORMALIZED_SOURCE_DIR MATCHES "unit")
        list(APPEND TEST_LABELS "unit")
    endif()
    if(NORMALIZED_SOURCE_DIR MATCHES "integration")
        list(APPEND TEST_LABELS "integration")
    endif()
    if(NORMALIZED_SOURCE_DIR MATCHES "benchmark")
        list(APPEND TEST_LABELS "benchmark")
    endif()

    message(STATUS "Test ${TEST_NAME} labels: ${TEST_LABELS}")

    # Escape semicolons in labels list to pass it as a single property value
    string(REPLACE ";" "\\;" TEST_LABELS_ESCAPED "${TEST_LABELS}")

    # 启用测试发现并设置属性
    message(STATUS "Setting test ${TEST_NAME} working directory to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        TEST_PREFIX "${TEST_NAME}."
        PROPERTIES
            LABELS "${TEST_LABELS_ESCAPED}"
            TIMEOUT 0
    )

endfunction()

add_subdirectory(test_support)
add_subdirectory(common)
add_subdirectory(mocks)
add_subdirectory(helpers)
add_subdirectory(unit)
add_subdirectory(integration)

option(BUILD_BENCHMARK_TESTS "Build benchmark tests" OFF)
if(BUILD_BENCHMARK_TESTS)
    add_subdirectory(benchmark)
endif()

# 创建专用的内存泄漏测试目标
add_custom_target(memory_leak_test
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration --output-on-failure
    COMMENT "Running integration tests with memory leak detection"
)

