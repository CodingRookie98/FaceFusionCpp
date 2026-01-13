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
    message(STATUS "Setting test ${TEST_NAME} working directory to: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        PROPERTIES
            LABELS "facefusion"
            TIMEOUT 300
    )
endfunction()

add_subdirectory(unit)

