add_facefusion_test(
    inference_session_tests
    SOURCES
    inference_session_tests.cpp
    LINK_LIBRARIES
    foundation_ai
    foundation_infrastructure
)

add_facefusion_test(
    inference_session_registry_tests
    SOURCES
    inference_session_registry_tests.cpp
    LINK_LIBRARIES
    foundation_ai
    foundation_infrastructure
)
