add_facefusion_test(
    domain_frame_tests
    SOURCES
        enhancer/frame_enhancer_factory_test.cpp
        enhancer/frame_enhancer_impl_test.cpp

    LINK_LIBRARIES
        domain_frame
        foundation_infrastructure_test_support
        tests_test_support_foundation_ai_mock_inference_session
        ONNXRuntime::ONNXRuntime
        ONNX::onnx
        GTest::gmock
)
