add_facefusion_test(
    inference_session_tests
    SOURCES
    inference_session_tests.cpp
    LINK_LIBRARIES
    foundation_ai
    foundation_infrastructure
    foundation_infrastructure_test_support
)

add_facefusion_test(
    inference_session_registry_tests
    SOURCES
    inference_session_registry_tests.cpp
    LINK_LIBRARIES
    foundation_ai
    foundation_infrastructure
    foundation_infrastructure_test_support
)

add_facefusion_test(
    session_pool_tests
    SOURCES
    session_pool_tests.cpp
    LINK_LIBRARIES
    foundation_ai
    foundation_infrastructure
)
