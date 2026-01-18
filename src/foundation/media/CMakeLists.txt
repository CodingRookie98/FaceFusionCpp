add_modules_library(foundation_media)



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

copy_ffmpeg_binaries(foundation_media)
