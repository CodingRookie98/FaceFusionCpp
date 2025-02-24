cmake_minimum_required(VERSION 3.30)
project(FaceFusionCpp
        VERSION 0.32.1
        DESCRIPTION "This project is the C++ version of the open-source project FaceFusion."
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
            /utf-8
    )
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-static-libgcc -static-libstdc++)
endif ()

set(ProgramName ${PROJECT_NAME})
add_executable(${ProgramName})
if (LINUX)
    set_target_properties(${ProgramName} PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
endif ()

add_subdirectory(facefusionCpp)

find_package(OpenCV REQUIRED)
find_package(ONNX CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_package(spdlog CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(thread-pool CONFIG REQUIRED)
cmake_policy(SET CMP0167 NEW) # 抑制FindBoost 模块已被移除的警告
find_package(Boost REQUIRED COMPONENTS process)
find_package(FFMPEG REQUIRED)
target_include_directories(${ProgramName} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${ProgramName} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${ProgramName} PRIVATE ${FFMPEG_LIBRARIES})
target_include_directories(${ProgramName} PRIVATE ${SIMPLEINI_INCLUDE_DIRS})
target_link_libraries(${ProgramName} PRIVATE ${OpenCV_LIBS})

# make onnxruntime available
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnxruntime-gpu.cmake")
if (WIN32)
    file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/onnxruntime*.lib")
    foreach (ONNXRUNTIME_LIBRARY ${ONNXRUNTIME_LIBRARIES})
        target_link_libraries(${ProgramName} PRIVATE ${ONNXRUNTIME_LIBRARY})
    endforeach ()
endif ()
if (LINUX)
    file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/libonnxruntime*.so")
    foreach (ONNXRUNTIME_LIBRARY ${ONNXRUNTIME_LIBRARIES})
        target_link_libraries(${ProgramName} PRIVATE ${ONNXRUNTIME_LIBRARY})
    endforeach ()
endif ()
target_include_directories(${ProgramName} PRIVATE "${ORT_PATH}/include")

target_link_libraries(${ProgramName} PRIVATE ONNX::onnx ONNX::onnx_proto)
target_link_libraries(${ProgramName} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${ProgramName} PRIVATE spdlog::spdlog_header_only)
target_link_libraries(${ProgramName} PRIVATE CURL::libcurl)
target_link_libraries(${ProgramName} PRIVATE indicators::indicators)
target_link_libraries(${ProgramName} PRIVATE OpenSSL::Crypto)
target_link_libraries(${ProgramName} PRIVATE dp::thread-pool)
target_link_libraries(${ProgramName} PRIVATE Boost::process)

set(${ProgramName}_ini_file_name "FaceFusionCpp.ini")
list(APPEND RESOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/${${ProgramName}_ini_file_name}"
        "${CMAKE_CURRENT_SOURCE_DIR}/facefusionCpp/modelsInfo.json"
)
add_custom_target(copy_resource_files
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RESOURCE_FILES}
        $<TARGET_FILE_DIR:${ProgramName}>
        COMMENT "Copying resource files to runtime output directory"
)
add_dependencies(${ProgramName} copy_resource_files)

set(X_VCPKG_APPLOCAL_DEPS_INSTALL TRUE CACHE BOOL "Automatically COPY dependencies into the install target directory for executables.")
set(X_VCPKG_APPLOCAL_DEPS_SERIALIZED ON) # 控制VCPKG的依赖项是否被序列化处理
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
    cmake_policy(SET CMP0177 NEW) #使用CMP0177策略的新行为，即对DESTINATION路径进行规范化处理, 需要cmake>=3.31
endif ()
install(TARGETS ${ProgramName} DESTINATION "."
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION "."
        ARCHIVE DESTINATION "."
        COMPONENT "app"
)

if (WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${ProgramName}> COMPONENT "app_dll" DESTINATION ".")
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

set(INSTALL_INI_FILE "${CMAKE_INSTALL_PREFIX}/${${ProgramName}_ini_file_name}")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -D INI_FILE_PATH=${INSTALL_INI_FILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clean_ini_file.cmake)")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)

find_program(CMAKE_EXE cmake)
add_custom_target(install_all
        COMMAND ${CMAKE_EXE} --install ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(install_all ${ProgramName})

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

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(file_${ProgramName}_ini "${CPACK_PACKAGE_DIRECTORY}/_CPack_Packages/win64/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/${${ProgramName}_ini_file_name}")
    else ()
        set(file_${ProgramName}_ini "${CPACK_PACKAGE_DIRECTORY}/_CPack_Packages/win32/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/${${ProgramName}_ini_file_name}")
    endif ()
else ()
    set(file_${ProgramName}_ini "${CPACK_PACKAGE_DIRECTORY}/_CPack_Packages/${CMAKE_SYSTEM_NAME}/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/${${ProgramName}_ini_file_name}")
endif ()

set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clean_ini_file.cmake")
include(CPack)
find_program(CPACK_EXE NAMES cpack)
add_custom_target(cpack_all
        COMMAND ${CPACK_EXE} -V --config CPackConfig.cmake -D INI_FILE_PATH=${file_${ProgramName}_ini}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(cpack_all install_all)