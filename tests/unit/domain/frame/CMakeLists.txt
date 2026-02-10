add_facefusion_test(
    domain_frame_tests
    SOURCES
        enhancer/frame_enhancer_factory_test.cpp
        enhancer/frame_enhancer_impl_test.cpp

    LINK_LIBRARIES
        domain_frame
        test_helpers
        test_mocks
        ONNXRuntime::ONNXRuntime
        ONNX::onnx
        GTest::gmock
)
