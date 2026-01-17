add_modules_library(domain_pipeline)

find_package(OpenCV REQUIRED)

target_sources(domain_pipeline
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            pipeline.ixx
            pipeline_types.ixx
            pipeline_api.ixx
            pipeline_adapters.ixx
            queue.ixx
            impl/pipeline_impl.ixx
)

target_link_libraries(domain_pipeline
    PUBLIC
        foundation_infrastructure
        domain_face_swapper
        domain_face_enhancer
        domain_face_expression
        domain_frame
        ${OpenCV_LIBS}
)


