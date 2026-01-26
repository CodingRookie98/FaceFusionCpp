# Pipeline Runner Service
# 配置驱动的 Pipeline 执行器

add_modules_library(services_pipeline)

find_package(OpenCV REQUIRED)

target_sources(services_pipeline
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            processors/face_analysis_processor.ixx
            pipeline_runner.ixx
            runner_video.cpp
            runner_image.cpp
            runner_types.cpp
)

target_sources(services_pipeline
    PRIVATE
        pipeline_runner.cpp
)

target_link_libraries(services_pipeline
    PUBLIC
        config_core
        config_parser
        domain_pipeline
    PRIVATE
        domain_face
        domain_face_analyser
        domain_face_detector
        domain_face_recognizer
        domain_face_masker
        domain_face_swapper
        domain_face_expression
        domain_frame
        domain_ai
        foundation_ai
        foundation_media
        ${OpenCV_LIBS}
)
