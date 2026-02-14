# cmake/version.cmake
# 版本信息注入脚本 - 通过 configure_file 生成 C++ 源文件
# 该脚本在配置阶段运行一次，并定义 custom_target 在每次构建时更新版本信息

# 定义生成版本的命令
# 注意：使用 list(APPEND) 构建命令列表，避免字符串拼接导致的参数解析问题
set(GENERATE_VERSION_CMD
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -DPROJECT_NAME=${PROJECT_NAME}
    -DPROJECT_VERSION=${PROJECT_VERSION}
    -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    -P ${CMAKE_CURRENT_LIST_DIR}/GenerateVersion.cmake
)

# 1. 在配置阶段立即生成一次 (确保文件存在)
execute_process(
    COMMAND ${GENERATE_VERSION_CMD}
    RESULT_VARIABLE RET
)
if(NOT RET EQUAL 0)
    message(WARNING "Failed to generate version.cpp during configuration")
endif()

# 2. 定义自定义目标，在构建时总是运行
add_custom_target(update_version_info
    COMMAND ${GENERATE_VERSION_CMD}
    BYPRODUCTS ${CMAKE_BINARY_DIR}/generated/version.cpp
    COMMENT "Updating version information..."
    VERBATIM
)

message(STATUS "Configured version.cpp generation (configure-time and build-time)")
