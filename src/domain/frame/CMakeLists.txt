find_package(OpenCV REQUIRED)

add_modules_library(domain_frame)

target_link_libraries(domain_frame
    PUBLIC
        ${OpenCV_LIBS}
        foundation_ai
        domain_ai
        foundation_media
)

add_subdirectory(enhancer)
