# cmake/dependencies.cmake

# --- OpenCV ---
if(NOT TARGET opencv_world)
    find_package(OpenCV REQUIRED)
    message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
endif()

# --- FFmpeg ---
if(NOT TARGET FFmpeg::avcodec)
    find_package(FFMPEG REQUIRED)
    message(STATUS "Found FFmpeg: ${FFMPEG_VERSION}")
endif()

# --- GTest (Test Only) ---
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    message(STATUS "Found GTest: ${GTest_VERSION}")
endif()

# --- Helper Function for Runtime DLLs ---
function(copy_ffmpeg_binaries TARGET_NAME)
    if(WIN32)
        # Verify FFMPEG_INCLUDE_DIRS is available
        if(NOT FFMPEG_INCLUDE_DIRS)
            message(WARNING "FFMPEG_INCLUDE_DIRS not set, cannot locate DLLs for copying.")
            return()
        endif()

        # Determine root based on include dir
        # FFMPEG_INCLUDE_DIRS[0] -> .../vcpkg_installed/x64-windows/include
        list(GET FFMPEG_INCLUDE_DIRS 0 _FFMPEG_INC)
        get_filename_component(_VCPKG_ROOT "${_FFMPEG_INC}" DIRECTORY)

        if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
            set(FFMPEG_BIN_SRC "${_VCPKG_ROOT}/debug/bin")
            set(X264_BIN_SRC "${_VCPKG_ROOT}/tools/x264/debug/bin")
            message(STATUS "[${TARGET_NAME}] Configuring for DEBUG: Copying DLLs from ${FFMPEG_BIN_SRC} and ${X264_BIN_SRC}")
        else()
            set(FFMPEG_BIN_SRC "${_VCPKG_ROOT}/bin")
            set(X264_BIN_SRC "${_VCPKG_ROOT}/tools/x264/bin")
            message(STATUS "[${TARGET_NAME}] Configuring for RELEASE: Copying DLLs from ${FFMPEG_BIN_SRC} and ${X264_BIN_SRC}")
        endif()

        file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_SRC}/*.dll")
        file(GLOB X264_DLLS "${X264_BIN_SRC}/*.dll")
        set(ALL_COPY_DLLS ${FFMPEG_DLLS} ${X264_DLLS})

        if(ALL_COPY_DLLS)
            # Use specific Output Directory if set, otherwise fallback to target property or standard location
            if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
                set(TARGET_OUTPUT_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>")
            else()
                set(TARGET_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
            endif()

            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ALL_COPY_DLLS}
                "${TARGET_OUTPUT_DIR}"
                COMMENT "Copying FFmpeg/x264 DLLs to output directory for ${TARGET_NAME}"
                VERBATIM
            )
        else()
            message(WARNING "No FFmpeg/x264 DLLs found to copy in ${FFMPEG_BIN_SRC} or ${X264_BIN_SRC}")
        endif()
    endif()
endfunction()
