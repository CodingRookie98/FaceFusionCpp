add_cxx_module_library(foundation_media)

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
)

target_link_libraries(foundation_media
    PUBLIC
        foundation_infrastructure
    PRIVATE
        ${OpenCV_LIBS}
        # FFmpeg libraries if needed, or if they are just invoking executables
)
