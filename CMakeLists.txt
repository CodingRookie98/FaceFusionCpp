cmake_minimum_required(VERSION 3.31)
project(faceFusionCpp
        VERSION 0.3.1
        DESCRIPTION "This project is the C++ version of the open-source project FaceFusion."
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnxruntime-gpu.cmake")
add_library(onnxruntime SHARED IMPORTED)
if (LINUX)
    set_target_properties(onnxruntime PROPERTIES
            IMPORTED_LOCATION "${ORT_PATH}/lib/libonnxruntime.so"
            INTERFACE_INCLUDE_DIRECTORIES "${ORT_PATH}/include"
    )
endif ()
target_include_directories(${ProgramName} PRIVATE "${ORT_PATH}/include")
target_link_directories(${ProgramName} PRIVATE "${ORT_PATH}/lib")
if (LINUX)
    target_link_libraries(${ProgramName} PRIVATE onnxruntime)
endif ()
if (WIN32)
    target_link_libraries(${ProgramName} PRIVATE "${ORT_PATH}/lib/onnxruntime.lib")
endif ()

target_link_libraries(${ProgramName} PRIVATE ONNX::onnx ONNX::onnx_proto)
target_link_libraries(${ProgramName} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${ProgramName} PRIVATE spdlog::spdlog_header_only)
target_link_libraries(${ProgramName} PRIVATE CURL::libcurl)
target_link_libraries(${ProgramName} PRIVATE indicators::indicators)
target_link_libraries(${ProgramName} PRIVATE OpenSSL::Crypto)
target_link_libraries(${ProgramName} PRIVATE dp::thread-pool)
target_link_libraries(${ProgramName} PRIVATE Boost::process)

list(APPEND RESOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/faceFusionCpp.ini"
        "${CMAKE_CURRENT_SOURCE_DIR}/facefusionCpp/modelsInfo.json"
)
add_custom_target(COPY_RESOURCE_FILES
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RESOURCE_FILES}
        $<TARGET_FILE_DIR:${ProgramName}>
        COMMENT "Copying resource files to runtime output directory"
)
add_dependencies(${ProgramName} COPY_RESOURCE_FILES)

set(X_VCPKG_APPLOCAL_DEPS_INSTALL TRUE CACHE BOOL "Automatically COPY dependencies into the install target directory for executables.")
set(X_VCPKG_APPLOCAL_DEPS_SERIALIZED ON) # 控制VCPKG的依赖项是否被序列化处理
cmake_policy(SET CMP0177 NEW) #使用CMP0177策略的新行为，即对DESTINATION路径进行规范化处理, 需要cmake>=3.31
install(TARGETS ${ProgramName} DESTINATION "./"
        RUNTIME DESTINATION "./"
        LIBRARY DESTINATION "./"
        ARCHIVE DESTINATION "./"
        COMPONENT "app"
)

if (WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${ProgramName}>
            COMPONENT "app_dll"
            DESTINATION "./"
    )
endif ()

install(FILES ${RESOURCE_FILES} DESTINATION "./" COMPONENT "app")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DOCUMENT.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/DISCLAIMER.txt"
        DESTINATION "./"
        COMPONENT "app"
)

# Please configure cmake again after building the project
if (WIN32)
    file(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.dll")
    file(GLOB OUTPUT_RUNTIME_LIBS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll")
endif ()
if (LINUX)
    file(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.so*")
endif ()
if (WIN32)
    install(FILES ${ONNXRUNTIME_LIBS} ${OUTPUT_RUNTIME_LIBS} DESTINATION "./" COMPONENT "app")
endif ()
if (LINUX)
    install(FILES ${ONNXRUNTIME_LIBS} ${OUTPUT_RUNTIME_LIBS} DESTINATION "./lib" COMPONENT "app")
endif ()

set(INSTALL_INI_FILE "${CMAKE_INSTALL_PREFIX}/faceFusionCpp.ini")
file(WRITE "${CMAKE_BINARY_DIR}/trim_file_content_4install.cmake" "
    file(READ \"${INSTALL_INI_FILE}\" CONTENT)
    string(REPLACE \"\n\" \";\" LINES \"\${CONTENT\}\") # 按行分隔
    set(NEW_CONTENT \"\")
    foreach(LINE IN LISTS LINES)
        string(REGEX REPLACE \"((.*)=).*\" \"\\\\1\" LINE \"\$\{LINE\}\") # 删除最后一个=后的内容
        set(NEW_CONTENT \"\$\{NEW_CONTENT\}\$\{LINE\}\\n\")
    endforeach()
    file(WRITE \"${INSTALL_INI_FILE}\" \"\$\{NEW_CONTENT\}\") # 写入文件
")
install(SCRIPT "${CMAKE_BINARY_DIR}/trim_file_content_4install.cmake" COMPONENT "app")

# "copy dependencies"
# add_custom_command(TARGET ${ProgramName} POST_BUILD
#     COMMAND ${CMAKE_SOURCE_DIR}/install_dependencies.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ProgramName}
# )
# install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib" COMPONENT "libs" DESTINATION "./")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "./")
include(InstallRequiredSystemLibraries)

# 设置打包的方式
if (WIN32)
    set(CPACK_GENERATOR ZIP)
endif ()
if (LINUX)
    set(CPACK_GENERATOR TGZ)
endif ()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${faceFusionCpp_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${faceFusionCpp_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${faceFusionCpp_VERSION_PATCH}")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${${PROJECT_NAME}_VERSION}-${CMAKE_BUILD_TYPE}")

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CPACK_INI_FILE "${CMAKE_BINARY_DIR}/_CPack_Packages/win64/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/faceFusionCpp.ini")
    else ()
        set(CPACK_INI_FILE "${CMAKE_BINARY_DIR}/_CPack_Packages/win32/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/faceFusionCpp.ini")
    endif ()
else ()
    set(CPACK_INI_FILE "${CMAKE_BINARY_DIR}/_CPack_Packages/${CMAKE_SYSTEM_NAME}/${CPACK_GENERATOR}/${CPACK_PACKAGE_FILE_NAME}/faceFusionCpp.ini")
endif ()
file(WRITE "${CMAKE_BINARY_DIR}/trim_file_content_4cpack.cmake" "
    file(READ \"${CPACK_INI_FILE}\" CONTENT)
    string(REPLACE \"\n\" \";\" LINES \"\${CONTENT\}\") # 按行分隔
    set(NEW_CONTENT \"\")
    foreach(LINE IN LISTS LINES)
        string(REGEX REPLACE \"((.*)=).*\" \"\\\\1\" LINE \"\$\{LINE\}\") # 删除最后一个=后的内容
        set(NEW_CONTENT \"\$\{NEW_CONTENT\}\$\{LINE\}\\n\")
    endforeach()
    file(WRITE \"${CPACK_INI_FILE}\" \"\$\{NEW_CONTENT\}\") # 写入文件
")
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_BINARY_DIR}/trim_file_content_4cpack.cmake")
include(CPack)
