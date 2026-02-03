set(ORT_VERSION_STR "1.20.1")
set(THIRD_PARTY_DIR "${CMAKE_BINARY_DIR}/third_party")

# --- Download Configuration ---
if (LINUX)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-linux-x64-gpu-${ORT_VERSION_STR}.tgz")
    set(ORT_FILE_BASE_NAME "onnxruntime-linux-x64-gpu-${ORT_VERSION_STR}")
elseif (WIN32 OR WIN64)
    set(ORT_DOWNLOAD_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION_STR}/onnxruntime-win-x64-gpu-${ORT_VERSION_STR}.zip")
    set(ORT_FILE_BASE_NAME "onnxruntime-win-x64-gpu-${ORT_VERSION_STR}")
endif ()

get_filename_component(ORT_FILE_NAME "${ORT_DOWNLOAD_URL}" NAME)
set(ORT_DOWNLOAD_FILE "${CMAKE_BINARY_DIR}/${ORT_FILE_NAME}")
set(ORT_EXTRACT_DIR "${THIRD_PARTY_DIR}")
set(ORT_PATH "${ORT_EXTRACT_DIR}/${ORT_FILE_BASE_NAME}")

# --- Download and Extract ---
if (NOT EXISTS "${ORT_PATH}")
    if (NOT EXISTS "${ORT_DOWNLOAD_FILE}")
        message(STATUS "Downloading ONNX Runtime v${ORT_VERSION_STR} from ${ORT_DOWNLOAD_URL}...")
        file(DOWNLOAD "${ORT_DOWNLOAD_URL}" "${ORT_DOWNLOAD_FILE}" SHOW_PROGRESS)
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

# --- Import Target ---
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
    elseif (LINUX)
        # On Linux, we copy all .so files (including providers) to the output directory
        # so they can be loaded as plugins at runtime.
        file(GLOB ORT_RUNTIME_LIBS "${ORT_PATH}/lib/*.so*")
    endif ()

    if (ORT_RUNTIME_LIBS)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ORT_RUNTIME_LIBS}
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMENT "Copying ONNX Runtime runtime libraries for ${TARGET_NAME}"
        )
    endif ()
endfunction()
