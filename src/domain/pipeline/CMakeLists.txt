add_modules_library(domain_pipeline)

find_package(OpenCV REQUIRED)

target_sources(domain_pipeline
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            pipeline.ixx
            pipeline_types.ixx
            queue.ixx
)

target_link_libraries(domain_pipeline
    PUBLIC
        foundation_infrastructure
        # domain_face # 暂时注释，避免循环依赖或未找到定义，等后续添加
        ${OpenCV_LIBS}
)

