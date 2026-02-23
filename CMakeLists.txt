cmake_minimum_required(VERSION 3.30)
project(FaceFusionCpp
        VERSION 0.34.1
        DESCRIPTION "This project is the C++ version of the open-source project FaceFusion."
)

include(cmake/version.cmake)

# --- 路径与输出目录配置 ---
# 注意：使用 CMakePresets.json 构建时，这些变量会被 Preset 中的 cacheVariables 覆盖，
# 指向 ${sourceDir}/build/bin/<preset> 目录，以防止 clean 操作误删二进制文件。
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()
# 确保在 Multi-config 生成器下也能正常工作
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
endforeach()
# -------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 强制使用 strict C++20，避免 PCM 匹配错误

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-static-libgcc -static-libstdc++)
endif ()

set(app_facefusioncpp "FaceFusionCpp")
add_executable(${app_facefusioncpp})
if (LINUX)
    set_target_properties(${app_facefusioncpp} PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
endif ()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

target_link_libraries(${app_facefusioncpp} PRIVATE project_options)

# make onnxruntime available
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnxruntime-gpu.cmake")

# 添加测试支持
enable_testing()

# 配置 GoogleTest 依赖（使用 vcpkg）
option(BUILD_TESTING "Build tests" ON)

find_package(Python3 REQUIRED)

include(cmake/dependencies.cmake)

# Find CUDA Toolkit to get version for package naming
find_package(CUDAToolkit)
if (CUDAToolkit_FOUND)
    message(STATUS "CUDA Toolkit found: ${CUDAToolkit_VERSION} (Major: ${CUDAToolkit_VERSION_MAJOR})")
endif()

# --- OpenCV C++20 Compatibility Patch ---
# This is an enterprise-grade workaround for an ambiguity in OpenCV 4.x Rect class
# when compiled with C++20 standard.
set(VCPKG_INSTALLED_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed")
if (EXISTS "${VCPKG_INSTALLED_DIR}")
    message(STATUS "Checking for OpenCV C++20 compatibility patch...")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/patch_opencv.py" "${VCPKG_INSTALLED_DIR}"
        RESULT_VARIABLE PATCH_RESULT
    )
    if (PATCH_RESULT EQUAL 0)
        message(STATUS "OpenCV patch check completed.")
    else ()
        message(WARNING "OpenCV patch script failed with exit code ${PATCH_RESULT}.")
    endif ()
endif ()
include(cmake/resources.cmake)
include(cmake/options.cmake)
include(cmake/modules_lib.cmake)

add_subdirectory(src)

if(BUILD_TESTING)
    # find_package(GTest) moved to cmake/dependencies.cmake

    # 添加测试子目录
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# 添加静态分析工具
option(ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set_target_properties(${app_facefusioncpp} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}"
        )
    endif()
endif()

# 添加代码覆盖率支持
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling coverage for GCC/Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    elseif(MSVC)
        message(STATUS "Enabling debug info for MSVC (use OpenCppCoverage)")
        add_compile_options(/DEBUG)
    endif()
endif()


# 添加性能优化选项
if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()


target_link_libraries(${app_facefusioncpp} PRIVATE foundation_ai)
target_link_libraries(${app_facefusioncpp} PRIVATE foundation_media)
target_link_libraries(${app_facefusioncpp} PRIVATE domain_ai)

copy_onnxruntime_libs(${app_facefusioncpp})
setup_resource_copying(${app_facefusioncpp})

set(X_VCPKG_APPLOCAL_DEPS_INSTALL TRUE CACHE BOOL "Automatically COPY dependencies into the install target directory for executables.")
set(X_VCPKG_APPLOCAL_DEPS_SERIALIZED ON) # 控制VCPKG的依赖项是否被序列化处理
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
    cmake_policy(SET CMP0177 NEW) #使用CMP0177策略的新行为，即对DESTINATION路径进行规范化处理, 需要cmake>=3.31
endif ()
install(TARGETS ${app_facefusioncpp} DESTINATION "."
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION "."
        ARCHIVE DESTINATION "."
        COMPONENT "app"
)

if (WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${app_facefusioncpp}> COMPONENT "app_dll" DESTINATION ".")
endif ()

install(FILES ${RESOURCE_FILES} DESTINATION "." COMPONENT "resource")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/README_CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DISCLAIMER.txt"
        DESTINATION "."
        COMPONENT "doc"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/config/app_config.yaml"
        "${CMAKE_CURRENT_SOURCE_DIR}/config/task_config.yaml"
        DESTINATION "config"
        COMPONENT "config"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/user"
        DESTINATION "docs"
        COMPONENT "doc"
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            DESTINATION "."
            COMPONENT "resource"
    )
endif()

# Ensure assets/models directory structure exists in the package
install(DIRECTORY DESTINATION "assets/models" COMPONENT "resource")

# Please configure cmake again after building the project
if (WIN32)
    file(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.dll")
    file(GLOB OUTPUT_RUNTIME_LIBS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll")
endif ()
if (LINUX)
    file(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.so*")
    file(GLOB OUTPUT_RUNTIME_LIBS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.so")
endif ()
if (WIN32)
    install(FILES ${ONNXRUNTIME_LIBS} ${OUTPUT_RUNTIME_LIBS} DESTINATION "." COMPONENT "app_libs")
endif ()
if (LINUX)
    install(FILES ${ONNXRUNTIME_LIBS} ${OUTPUT_RUNTIME_LIBS} DESTINATION "." COMPONENT "app_libs")
endif ()

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

find_program(CMAKE_EXE cmake)
add_custom_target(install_all
        COMMAND ${CMAKE_EXE} --install ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(install_all ${app_facefusioncpp})

# 设置打包的方式
if (WIN32)
    set(CPACK_GENERATOR 7Z)
endif ()
if (LINUX)
    set(CPACK_GENERATOR TXZ)
    set(CPACK_ARCHIVE_COMPRESSION "xz")
endif ()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${${PROJECT_NAME}_VERSION_PATCH}")
set(CPACK_PACKAGE_NAME "FaceFusionCpp")

# Get CUDA version major for package name suffix
if (DEFINED CUDAToolkit_VERSION_MAJOR)
    set(CUDA_SUFFIX "-CUDA${CUDAToolkit_VERSION_MAJOR}")
else()
    set(CUDA_SUFFIX "")
endif()

# Customize System Name for Linux to be distro-specific if needed
if (LINUX)
    # Detect distro? For now, we hardcode to Ubuntu24.04 as we build on it and rely on its glibc.
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    string(REGEX MATCH "ID=([a-zA-Z0-9]+)" _ ${OS_RELEASE_CONTENT})
    set(OS_ID ${CMAKE_MATCH_1})
    string(REGEX MATCH "VERSION_ID=\"?([0-9]+\\.[0-9]+)\"?" _ ${OS_RELEASE_CONTENT})
    set(OS_VERSION_ID ${CMAKE_MATCH_1})

    if (OS_ID AND OS_VERSION_ID)
        # Capitalize first letter of ID (e.g. ubuntu -> Ubuntu)
        string(SUBSTRING ${OS_ID} 0 1 FIRST_CHAR)
        string(TOUPPER ${FIRST_CHAR} FIRST_CHAR)
        string(SUBSTRING ${OS_ID} 1 -1 REMAINING_CHARS)
        set(OS_ID_CAP "${FIRST_CHAR}${REMAINING_CHARS}")

        set(OS_DISTRO_NAME "${OS_ID_CAP}${OS_VERSION_ID}")
    else()
        set(OS_DISTRO_NAME "Linux")
    endif()
else()
    set(OS_DISTRO_NAME "${CMAKE_SYSTEM_NAME}")
endif()

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${${PROJECT_NAME}_VERSION}-${OS_DISTRO_NAME}-${CMAKE_SYSTEM_PROCESSOR}${CUDA_SUFFIX}")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_THREADS 0) # Use multi-threads. Given 0 CPack will try to use all available CPU cores.
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/packages/${CMAKE_BUILD_TYPE}")

include(CPack)