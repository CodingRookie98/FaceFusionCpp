add_modules_library(foundation_media)

find_package(OpenCV REQUIRED)
find_package(FFMPEG REQUIRED)

target_sources(foundation_media
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            vision.ixx
            ffmpeg.ixx
    PRIVATE
        vision.cpp
        ffmpeg.cpp
        ffmpeg_reader.cpp
        ffmpeg_writer.cpp
)

target_link_libraries(foundation_media
    PUBLIC
        foundation_infrastructure
        ${OpenCV_LIBS}
        ${FFMPEG_LIBRARIES}
)

target_include_directories(foundation_media PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_directories(foundation_media PUBLIC ${FFMPEG_LIBRARY_DIRS})

if (WIN32)
    # Determine root based on include dir (shared between debug/release)
    # FFMPEG_INCLUDE_DIRS[0] -> .../vcpkg_installed/x64-windows/include
    list(GET FFMPEG_INCLUDE_DIRS 0 _FFMPEG_INC)
    get_filename_component(_VCPKG_ROOT "${_FFMPEG_INC}" DIRECTORY)

    if (CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
        set(FFMPEG_BIN_SRC "${_VCPKG_ROOT}/debug/bin")
        set(X264_BIN_SRC "${_VCPKG_ROOT}/tools/x264/debug/bin")
        message(STATUS "Configuring for DEBUG: Copying DLLs from ${FFMPEG_BIN_SRC} and ${X264_BIN_SRC}")
    else()
        set(FFMPEG_BIN_SRC "${_VCPKG_ROOT}/bin")
        set(X264_BIN_SRC "${_VCPKG_ROOT}/tools/x264/bin")
        message(STATUS "Configuring for RELEASE: Copying DLLs from ${FFMPEG_BIN_SRC} and ${X264_BIN_SRC}")
    endif()

    file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_SRC}/*.dll")
    file(GLOB X264_DLLS "${X264_BIN_SRC}/*.dll")

    set(ALL_COPY_DLLS ${FFMPEG_DLLS} ${X264_DLLS})

    if (ALL_COPY_DLLS)
        add_custom_command(TARGET foundation_media POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ALL_COPY_DLLS}
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMENT "Copying FFmpeg/x264 DLLs to runtime output directory"
            VERBATIM
        )
    else()
        message(WARNING "No FFmpeg/x264 DLLs found to copy. Check path: ${_VCPKG_ROOT}")
    endif()
endif()