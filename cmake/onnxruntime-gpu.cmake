set(THIRD_PARTY_DIR "${CMAKE_BINARY_DIR}/third_party")

set(ORT_VERSION_STR "1.20.1")
# 下载文件的 URL 和目标文件名
set(ORT_DOWNLOAD_DIR "")
if (WIN32 OR WIN64)
    if (DEFINED ENV{TEMP})
        set(ORT_DOWNLOAD_DIR "$ENV{TEMP}")
    elseif (DEFINED ENV{TMP})
        set(ORT_DOWNLOAD_DIR "$ENV{TMP}")
    endif ()
elseif (UNIX)
    if (DEFINED ENV{TMPDIR})
        set(ORT_DOWNLOAD_DIR "$ENV{TMPDIR}")
    else ()
        set(ORT_DOWNLOAD_DIR "/tmp")
    endif ()
endif ()
if (LINUX)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-linux-x64-gpu-${ORT_VERSION_STR}.tgz")
    string(REGEX MATCH "([^/]+)\\.tgz$" ORT_FILE_NAME ${ORT_DOWNLOAD_URL})
    string(REGEX REPLACE "\\.tgz$" "" ORT_FILE_BASE_NAME ${ORT_FILE_NAME})
elseif (WIN32 OR WIN64)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-win-x64-gpu-${ORT_VERSION_STR}.zip")
    string(REGEX MATCH "([^/]+)\\.zip$" ORT_FILE_NAME ${ORT_DOWNLOAD_URL})
    string(REGEX REPLACE "\\.zip$" "" ORT_FILE_BASE_NAME ${ORT_FILE_NAME})
endif ()
set(ORT_DOWNLOAD_FILE "${ORT_DOWNLOAD_DIR}/${ORT_FILE_NAME}")

set(ORT_EXTRACT_DIR "${THIRD_PARTY_DIR}")

if (NOT EXISTS "${ORT_EXTRACT_DIR}")
    # 确保解压目录存在
    file(MAKE_DIRECTORY "${ORT_EXTRACT_DIR}")
endif ()

# 检查是否已经下载
if (NOT EXISTS "${ORT_DOWNLOAD_FILE}")
    # 确保下载目录存在
    file(MAKE_DIRECTORY "${ORT_DOWNLOAD_DIR}")

    # 下载文件
    message(STATUS "Downloading ${ORT_FILE_NAME}")
    file(DOWNLOAD "${ORT_DOWNLOAD_URL}" "${ORT_DOWNLOAD_FILE}.tmp" SHOW_PROGRESS)
    # 重命名临时文件为最终文件名
    file(RENAME "${ORT_DOWNLOAD_FILE}.tmp" "${ORT_DOWNLOAD_FILE}")
else ()
    message(STATUS "${ORT_FILE_NAME} has already been downloaded.")
endif ()

set(ORT_PATH "${ORT_EXTRACT_DIR}/${ORT_FILE_BASE_NAME}")
if (NOT EXISTS "${ORT_PATH}")
    # 解压文件
    message(STATUS "Extracting ${ORT_DOWNLOAD_FILE}")
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${ORT_DOWNLOAD_FILE}"
            WORKING_DIRECTORY "${ORT_EXTRACT_DIR}"
    )
else ()
    message(STATUS "${ORT_FILE_NAME} has already been extracted.")
endif ()

if (WIN32 OR WIN64)
    FILE(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.dll")
elseif (LINUX)
    FILE(GLOB ONNXRUNTIME_LIBS "${ORT_PATH}/lib/*.so*")
endif ()

add_custom_command(TARGET ${app_facefusioncpp} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ONNXRUNTIME_LIBS}
        $<TARGET_FILE_DIR:${app_facefusioncpp}>
        COMMENT "Copying ONNX Runtime libs to runtime output directory"
)

if (NOT TARGET ONNXRuntime::ONNXRuntime)
    add_library(ONNXRuntime::ONNXRuntime INTERFACE IMPORTED GLOBAL)
    target_include_directories(ONNXRuntime::ONNXRuntime INTERFACE "${ORT_PATH}/include")

    if (WIN32)
        file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/onnxruntime*.lib")
        target_link_libraries(ONNXRuntime::ONNXRuntime INTERFACE ${ONNXRUNTIME_LIBRARIES})
    elseif (LINUX)
        file(GLOB ONNXRUNTIME_LIBRARIES "${ORT_PATH}/lib/libonnxruntime*.so")
        target_link_libraries(ONNXRuntime::ONNXRuntime INTERFACE ${ONNXRUNTIME_LIBRARIES})
    endif ()
endif()
