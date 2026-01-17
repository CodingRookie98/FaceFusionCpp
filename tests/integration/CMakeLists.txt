add_facefusion_test(
    app_test_pipeline
    SOURCES
        app/pipeline_integration_test.cpp
    LINK_LIBRARIES
        domain_pipeline
        domain_face
        domain_face_detector
        domain_face_recognizer # Is this a lib?
        domain_face_swapper
        domain_ai
        foundation_ai
        foundation_infrastructure
        ${OpenCV_LIBS}
)
