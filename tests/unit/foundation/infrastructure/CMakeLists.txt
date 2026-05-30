add_facefusion_test(
    core_utils_tests
    SOURCES
    core_utils_test.cpp
    LINK_LIBRARIES
    foundation_infrastructure
)

add_facefusion_test(
    file_system_tests
    SOURCES
    file_system_test.cpp
    LINK_LIBRARIES
    foundation_infrastructure
    test_helpers
    test_common
)

add_facefusion_test(
    crypto_tests
    SOURCES
    crypto_test.cpp
    LINK_LIBRARIES
    foundation_infrastructure
    test_common
)


add_facefusion_test(
        thread_pool_tests
        SOURCES
        thread_pool_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        logger_tests
        SOURCES
        logger_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        concurrent_crypto_tests
        SOURCES
        concurrent_crypto_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
        test_common
)

add_facefusion_test(
        progress_tests
        SOURCES
        progress_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        process_tests
        SOURCES
        process_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        cuda_utils_tests
        SOURCES
        cuda_utils_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        scoped_timer_test
        SOURCES
        scoped_timer_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        concurrent_queue_tests
        SOURCES
        concurrent_queue_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        network_test
        SOURCES
        network_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        console_integration_test
        SOURCES
        console_integration_test.cpp
        LINK_LIBRARIES
        foundation_infrastructure
        test_common
)
