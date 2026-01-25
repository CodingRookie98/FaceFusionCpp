add_facefusion_test(
    app_test_pipeline
    SOURCES
        app/pipeline_integration_test.cpp
        app/pipeline_runner_video_test.cpp
        app/pipeline_runner_image_test.cpp
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
        foundation_media
        ${OpenCV_LIBS}
)


# Ensure ONNX Runtime DLLs are copied to the test executable directory
copy_onnxruntime_libs(app_test_pipeline)

