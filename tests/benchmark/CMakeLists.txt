add_facefusion_test(
    app_benchmark_pipeline
    SOURCES
        app/pipeline_benchmark.cpp
    LINK_LIBRARIES
        domain_pipeline
        services_pipeline
        domain_face
        domain_face_detector
        domain_face_masker
        domain_face_swapper
        domain_face_expression
        domain_face_test_support
        domain_face_analyser
        domain_ai
        foundation_ai
        foundation_infrastructure
        test_helpers
        foundation_media
        ${OpenCV_LIBS}
)

# Benchmark specific properties
set_tests_properties(app_benchmark_pipeline PROPERTIES LABELS "benchmark")
set_tests_properties(app_benchmark_pipeline PROPERTIES TIMEOUT 600)

# Ensure ONNX Runtime DLLs are copied
if(COMMAND copy_onnxruntime_libs)
    copy_onnxruntime_libs(app_benchmark_pipeline)
endif()
