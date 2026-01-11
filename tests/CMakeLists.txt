cmake_minimum_required(VERSION 3.30)

# 测试目录的 CMakeLists.txt
# 此文件用于管理所有测试目标

# 包含 GoogleTest 模块
include(GoogleTest)

# 辅助函数：创建测试可执行文件
# 参数：
#   TEST_NAME: 测试名称
#   SOURCES: 测试源文件列表
#   LINK_LIBRARIES: 需要链接的库（可选）
function(add_facefusion_test TEST_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        ""
        "SOURCES;LINK_LIBRARIES"
    )

    # 创建测试可执行文件
    add_executable(${TEST_NAME} ${ARG_SOURCES})

    # 链接 GoogleTest 主函数
    target_link_libraries(${TEST_NAME} PRIVATE GTest::gtest_main)

    # 链接额外的库
    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${TEST_NAME} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # 启用测试发现并设置属性
    gtest_discover_tests(${TEST_NAME}
        PROPERTIES
            LABELS "facefusion"
            TIMEOUT 300
    )
endfunction()

# 示例测试：基础功能测试
add_facefusion_test(
    example_test
    SOURCES
        example_test.cpp
)

# Core Utils 模块测试
# Core Utils Module Tests
add_facefusion_test(
        core_utils_tests
        SOURCES
        unit/foundation/infrastructure/core_utils_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

# File System Module Tests
add_facefusion_test(
        file_system_tests
        SOURCES
        unit/foundation/infrastructure/file_system_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

# Crypto Module Tests
add_facefusion_test(
        crypto_tests
        SOURCES
        unit/foundation/infrastructure/crypto_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

# Network Module Tests
add_facefusion_test(
        network_tests
        SOURCES
        unit/foundation/infrastructure/network_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

# Model Manager Module Tests
add_facefusion_test(
    model_manager_tests
    SOURCES
    unit/domain/ai/model_manager_tests.cpp
    LINK_LIBRARIES
    domain_ai
    foundation_infrastructure
)
