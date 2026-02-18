set(ORT_VERSION_STR "1.24.1")
set(THIRD_PARTY_DIR "${CMAKE_BINARY_DIR}/third_party")
# Use a shared downloads directory inside build/ to persist across clean builds
set(DOWNLOADS_DIR "${CMAKE_SOURCE_DIR}/build/downloads")
file(MAKE_DIRECTORY "${DOWNLOADS_DIR}")

# --- Download Configuration ---
if (LINUX)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-linux-x64-gpu-${ORT_VERSION_STR}.tgz")
    set(ORT_FILE_BASE_NAME "onnxruntime-linux-x64-gpu-${ORT_VERSION_STR}")
elseif (WIN32 OR WIN64)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-win-x64-gpu-${ORT_VERSION_STR}.zip")
    set(ORT_FILE_BASE_NAME "onnxruntime-win-x64-gpu-${ORT_VERSION_STR}")
endif ()

get_filename_component(ORT_FILE_NAME "${ORT_DOWNLOAD_URL}" NAME)
set(ORT_DOWNLOAD_FILE "${DOWNLOADS_DIR}/${ORT_FILE_NAME}")
set(ORT_EXTRACT_DIR "${THIRD_PARTY_DIR}")
set(ORT_PATH "${ORT_EXTRACT_DIR}/${ORT_FILE_BASE_NAME}")

# --- Verify Installation Integrity ---
set(ORT_INTEGRITY_CHECK_FILE "${ORT_PATH}/include/onnxruntime_c_api.h")
if (WIN32)
    list(APPEND ORT_INTEGRITY_CHECK_FILE "${ORT_PATH}/lib/onnxruntime.lib")
    list(APPEND ORT_INTEGRITY_CHECK_FILE "${ORT_PATH}/lib/onnxruntime.dll")
elseif (APPLE)
    list(APPEND ORT_INTEGRITY_CHECK_FILE "${ORT_PATH}/lib/libonnxruntime.dylib")
else ()
    list(APPEND ORT_INTEGRITY_CHECK_FILE "${ORT_PATH}/lib/libonnxruntime.so")
endif ()

set(ORT_IS_INSTALLED TRUE)
foreach(CHECK_FILE ${ORT_INTEGRITY_CHECK_FILE})
    if (NOT EXISTS "${CHECK_FILE}")
        set(ORT_IS_INSTALLED FALSE)
        message(STATUS "Missing ONNX Runtime file: ${CHECK_FILE}")
        break()
    endif ()
endforeach()

if (NOT ORT_IS_INSTALLED AND EXISTS "${ORT_PATH}")
    message(WARNING "ONNX Runtime installation at ${ORT_PATH} is incomplete or corrupted. Removing and re-downloading...")
    file(REMOVE_RECURSE "${ORT_PATH}")
    # Also remove the downloaded archive to force a fresh download, as it might be corrupted
    if (EXISTS "${ORT_DOWNLOAD_FILE}")
        file(REMOVE "${ORT_DOWNLOAD_FILE}")
    endif ()
endif ()

# --- Download and Extract ---
if (NOT EXISTS "${ORT_PATH}")
    if (NOT EXISTS "${ORT_DOWNLOAD_FILE}")
        set(ORT_DOWNLOAD_FILE_TMP "${ORT_DOWNLOAD_FILE}.tmp")
        message(STATUS "Downloading ONNX Runtime v${ORT_VERSION_STR} from ${ORT_DOWNLOAD_URL}...")
        message(STATUS "If download fails, you can manually download it to ${ORT_DOWNLOAD_FILE}")

        # Clean up any previous partial download
        if (EXISTS "${ORT_DOWNLOAD_FILE_TMP}")
            file(REMOVE "${ORT_DOWNLOAD_FILE_TMP}")
        endif()

        file(DOWNLOAD "${ORT_DOWNLOAD_URL}" "${ORT_DOWNLOAD_FILE_TMP}"
            SHOW_PROGRESS
            STATUS ORT_DOWNLOAD_STATUS
        )
        list(GET ORT_DOWNLOAD_STATUS 0 ORT_DOWNLOAD_CODE)
        list(GET ORT_DOWNLOAD_STATUS 1 ORT_DOWNLOAD_MSG)
        if (NOT ORT_DOWNLOAD_CODE EQUAL 0)
            file(REMOVE "${ORT_DOWNLOAD_FILE_TMP}")
            message(FATAL_ERROR "Error downloading ONNX Runtime: ${ORT_DOWNLOAD_MSG}")
        else()
             file(RENAME "${ORT_DOWNLOAD_FILE_TMP}" "${ORT_DOWNLOAD_FILE}")
        endif ()
    endif ()

    message(STATUS "Extracting ${ORT_DOWNLOAD_FILE} to ${ORT_EXTRACT_DIR}...")
    file(MAKE_DIRECTORY "${ORT_EXTRACT_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${ORT_DOWNLOAD_FILE}"
        WORKING_DIRECTORY "${ORT_EXTRACT_DIR}"
        RESULT_VARIABLE ORT_TAR_RESULT
    )
    if (NOT ORT_TAR_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract ONNX Runtime binaries.")
    endif ()
endif ()

# --- Import Target (Manual Setup) ---
if (NOT TARGET ONNXRuntime::ONNXRuntime)
    add_library(ONNXRuntime::ONNXRuntime INTERFACE IMPORTED GLOBAL)

    # Use find_path and find_library for better robustness
    find_path(ORT_INCLUDE_DIR onnxruntime_c_api.h
        PATHS "${ORT_PATH}/include"
        NO_DEFAULT_PATH
    )

    if (WIN32)
        find_library(ORT_LIB onnxruntime
            PATHS "${ORT_PATH}/lib"
            NO_DEFAULT_PATH
        )
    else ()
        find_library(ORT_LIB onnxruntime
            PATHS "${ORT_PATH}/lib"
            NO_DEFAULT_PATH
        )
    endif ()

    if (NOT ORT_INCLUDE_DIR OR NOT ORT_LIB)
        message(FATAL_ERROR "Could not find ONNX Runtime headers or library in ${ORT_PATH}")
    endif ()

    target_include_directories(ONNXRuntime::ONNXRuntime INTERFACE "${ORT_INCLUDE_DIR}")
    target_link_libraries(ONNXRuntime::ONNXRuntime INTERFACE "${ORT_LIB}")

    message(STATUS "Found ONNX Runtime: ${ORT_LIB} (include: ${ORT_INCLUDE_DIR})")
endif()

# --- Helper function for runtime dependencies ---
function(copy_onnxruntime_libs TARGET_NAME)
    if (WIN32 OR WIN64)
        file(GLOB ORT_RUNTIME_LIBS "${ORT_PATH}/lib/*.dll")
        if (ORT_RUNTIME_LIBS)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ORT_RUNTIME_LIBS}
                $<TARGET_FILE_DIR:${TARGET_NAME}>
                COMMENT "Copying ONNX Runtime runtime libraries for ${TARGET_NAME}"
            )
        endif ()
    elseif (LINUX)
        # On Linux, we copy all .so files (including providers) to the output directory
        # so they can be loaded as plugins at runtime.
        file(GLOB ORT_RUNTIME_LIBS "${ORT_PATH}/lib/*.so*")

        if (ORT_RUNTIME_LIBS)
            # Use cp -P to preserve symbolic links to avoid duplicating libraries
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND cp -P ${ORT_RUNTIME_LIBS} $<TARGET_FILE_DIR:${TARGET_NAME}>
                COMMENT "Copying ONNX Runtime runtime libraries for ${TARGET_NAME}"
            )
        endif ()
    endif ()
endfunction()
