cmake_minimum_required(VERSION 3.30)
project(FaceFusionCpp
        VERSION 0.33.0
        DESCRIPTION "This project is the C++ version of the open-source project FaceFusion."
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "3.31")
#set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
            /utf-8
            /EHsc
    )
    add_definitions(-D_WIN32_WINNT=0x0A00 -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-static-libgcc -static-libstdc++)
endif ()

set(target_app_facefusioncpp "FaceFusionCpp")
add_executable(${target_app_facefusioncpp})
if (LINUX)
    set_target_properties(${target_app_facefusioncpp} PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
endif ()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(MSVC)
    target_compile_options(${target_app_facefusioncpp} PRIVATE /W4)
    target_compile_options(${target_app_facefusioncpp} PRIVATE /permissive-)
else()
    target_compile_options(${target_app_facefusioncpp} PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(facefusionCpp)

# 添加测试支持
enable_testing()

# 配置 GoogleTest 依赖（使用 vcpkg）
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)

    # 添加测试子目录
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# 添加静态分析工具
#find_program(CLANG_TIDY clang-tidy)
#if(CLANG_TIDY)
#    set_target_properties(${target_app_facefusioncpp} PROPERTIES
#        CXX_CLANG_TIDY "${CLANG_TIDY}"
#    )
#endif()

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

find_package(OpenCV REQUIRED)
find_package(ONNX CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(thread-pool CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
cmake_policy(SET CMP0167 NEW) # 抑制FindBoost 模块已被移除的警告
find_package(Boost REQUIRED COMPONENTS process system)
target_link_libraries(${target_app_facefusioncpp} PRIVATE Boost::process Boost::system)
find_package(FFMPEG REQUIRED)
target_include_directories(${target_app_facefusioncpp} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${target_app_facefusioncpp} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${target_app_facefusioncpp} PRIVATE ${FFMPEG_LIBRARIES})
target_link_libraries(${target_app_facefusioncpp} PRIVATE ${OpenCV_LIBS})

# make onnxruntime available
include("${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/onnxruntime-gpu.cmake")
if (WIN32)
    file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/onnxruntime*.lib")
    foreach (ONNXRUNTIME_LIBRARY ${ONNXRUNTIME_LIBRARIES})
        target_link_libraries(${target_app_facefusioncpp} PRIVATE ${ONNXRUNTIME_LIBRARY})
    endforeach ()
endif ()
if (LINUX)
    file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/libonnxruntime*.so")
    foreach (ONNXRUNTIME_LIBRARY ${ONNXRUNTIME_LIBRARIES})
        target_link_libraries(${target_app_facefusioncpp} PRIVATE ${ONNXRUNTIME_LIBRARY})
    endforeach ()
endif ()
target_include_directories(${target_app_facefusioncpp} PRIVATE "${ORT_PATH}/include")

target_link_libraries(${target_app_facefusioncpp} PRIVATE ONNX::onnx ONNX::onnx_proto)
target_link_libraries(${target_app_facefusioncpp} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${target_app_facefusioncpp} PRIVATE spdlog::spdlog_header_only)
target_link_libraries(${target_app_facefusioncpp} PRIVATE CURL::libcurl)
target_link_libraries(${target_app_facefusioncpp} PRIVATE indicators::indicators)
target_link_libraries(${target_app_facefusioncpp} PRIVATE OpenSSL::Crypto)
target_link_libraries(${target_app_facefusioncpp} PRIVATE dp::thread-pool)
target_link_libraries(${target_app_facefusioncpp} PRIVATE Boost::process)
target_link_libraries(${target_app_facefusioncpp} PRIVATE yaml-cpp::yaml-cpp)

list(APPEND RESOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/facefusionCpp/modelsInfo.json"
)
add_custom_target(copy_resource_files
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RESOURCE_FILES}
        $<TARGET_FILE_DIR:${target_app_facefusioncpp}>
        COMMENT "Copying resource files to runtime output directory"
)
add_dependencies(${target_app_facefusioncpp} copy_resource_files)

set(X_VCPKG_APPLOCAL_DEPS_INSTALL TRUE CACHE BOOL "Automatically COPY dependencies into the install target directory for executables.")
set(X_VCPKG_APPLOCAL_DEPS_SERIALIZED ON) # 控制VCPKG的依赖项是否被序列化处理
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
    cmake_policy(SET CMP0177 NEW) #使用CMP0177策略的新行为，即对DESTINATION路径进行规范化处理, 需要cmake>=3.31
endif ()
install(TARGETS ${target_app_facefusioncpp} DESTINATION "."
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION "."
        ARCHIVE DESTINATION "."
        COMPONENT "app"
)

if (WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${target_app_facefusioncpp}> COMPONENT "app_dll" DESTINATION ".")
endif ()

install(FILES ${RESOURCE_FILES} DESTINATION "." COMPONENT "resource")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DOCUMENT.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DOCUMENT_CN.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DISCLAIMER.txt"
        DESTINATION "."
        COMPONENT "doc"
)

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
add_dependencies(install_all ${target_app_facefusioncpp})

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
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${${PROJECT_NAME}_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_THREADS 0) # Use multi-threads. Given 0 CPack will try to use all available CPU cores.
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/packages/${CMAKE_BUILD_TYPE}")

include(CPack)
find_program(CPACK_EXE NAMES cpack)
add_custom_target(cpack_all
        COMMAND ${CPACK_EXE} -V --config CPackConfig.cmake -D INI_FILE_PATH=${file_${target_app_facefusioncpp}_ini}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(cpack_all install_all)