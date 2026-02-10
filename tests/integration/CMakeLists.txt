add_facefusion_test(
    app_test_pipeline
    SOURCES
        app/pipeline_runner_video_test.cpp
        app/pipeline_runner_image_test.cpp
        app/pipeline_runner_metrics_test.cpp
        app/checkpoint_resume_test.cpp
        app/edge_cases_test.cpp
        app/memory_monitoring_test.cpp
    LINK_LIBRARIES
        integration_test_support
        domain_pipeline
        services_pipeline
        domain_face
        domain_face_detector
        domain_face_masker
        domain_face_swapper
        domain_face_expression
        domain_face_enhancer
        domain_face_test_support
        domain_face_analyser
        domain_ai
        foundation_ai
        foundation_infrastructure
        foundation_infrastructure_test_support
        foundation_media
        ${OpenCV_LIBS}
)

copy_onnxruntime_libs(app_test_pipeline)

if(HAVE_NVML)
    target_link_libraries(app_test_pipeline PRIVATE ${NVML_LIBRARY})
    target_compile_definitions(app_test_pipeline PRIVATE HAVE_NVML)
endif()

add_facefusion_test(
    component_integration_tests
    MAIN_LIB GTest::gmock_main
    SOURCES
        domain/face/detector/face_detector_test.cpp
        domain/face/swapper/face_swapper_test.cpp
        domain/face/enhancer/face_enhancer_test.cpp
        domain/face/recognizer/face_recognizer_test.cpp
        domain/face/masker/face_masker_test.cpp
        domain/face/landmarker/face_landmarker_test.cpp
        domain/face/classifier/face_classifier_test.cpp
        domain/face/expression/face_expression_test.cpp
        domain/face/analyser/face_analyser_integration_test.cpp
        domain/face/analyser/model_registry_test.cpp
        domain/ai/model_repository_test.cpp
        domain/frame/enhancer/frame_enhancer_test.cpp
        foundation/ai/inference_session_test.cpp
        foundation/ai/session_pool_test.cpp
        foundation/ai/inference_session_registry_test.cpp
        foundation/media/ffmpeg_test.cpp
        foundation/media/vision_test.cpp
        foundation/infrastructure/network_test.cpp
        domain/face/face_helper_test.cpp
        domain/face/face_selector_test.cpp
        domain/face/face_test.cpp
    LINK_LIBRARIES
        integration_test_support
        domain_face
        domain_face_detector
        domain_face_swapper
        domain_face_recognizer
        domain_face_masker
        domain_face_classifier
        domain_face_expression
        domain_face_enhancer
        domain_face_analyser
        domain_frame
        domain_ai
        foundation_ai
        foundation_media
        foundation_infrastructure
        domain_face_test_support
        foundation_infrastructure_test_support
        ${OpenCV_LIBS}
        GTest::gmock
)

copy_onnxruntime_libs(component_integration_tests)

if(HAVE_NVML)
    target_link_libraries(component_integration_tests PRIVATE ${NVML_LIBRARY})
    target_compile_definitions(component_integration_tests PRIVATE HAVE_NVML)
endif()
