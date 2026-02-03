# cmake/version.cmake
# 版本信息注入脚本 - 通过 configure_file 生成 C++ 源文件

# 获取构建时间
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)

# 获取 Git 信息 (可选)
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} symbolic-ref --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH "unknown")
endif()

# 确保生成的目录存在
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)

# 配置生成文件
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/version.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.cpp
    @ONLY
)

message(STATUS "Generated version.cpp with VERSION=${PROJECT_VERSION}, BUILD=${BUILD_TIMESTAMP}")
