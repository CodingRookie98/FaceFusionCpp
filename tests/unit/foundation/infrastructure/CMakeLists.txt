add_facefusion_test(
    core_utils_tests
    SOURCES
    core_utils_tests.cpp
    LINK_LIBRARIES
    foundation_infrastructure
)

add_facefusion_test(
    file_system_tests
    SOURCES
    file_system_tests.cpp
    LINK_LIBRARIES
    foundation_infrastructure
    foundation_infrastructure_test_support
)

add_facefusion_test(
    crypto_tests
    SOURCES
    crypto_tests.cpp
    LINK_LIBRARIES
    foundation_infrastructure
)


add_facefusion_test(
        thread_pool_tests
        SOURCES
        thread_pool_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        logger_tests
        SOURCES
        logger_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        concurrent_crypto_tests
        SOURCES
        concurrent_crypto_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        progress_tests
        SOURCES
        progress_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)

add_facefusion_test(
        process_tests
        SOURCES
        process_tests.cpp
        LINK_LIBRARIES
        foundation_infrastructure
)
