add_facefusion_test(
    vision_tests
    SOURCES
    vision_tests.cpp
    LINK_LIBRARIES
    foundation_media
    foundation_infrastructure
)

add_facefusion_test(
    ffmpeg_tests
    SOURCES
    ffmpeg_tests.cpp
    LINK_LIBRARIES
    foundation_media
    foundation_infrastructure
)
