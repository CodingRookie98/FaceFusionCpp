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
)

add_facefusion_test(
    crypto_tests
    SOURCES
    crypto_tests.cpp
    LINK_LIBRARIES
    foundation_infrastructure
)

add_facefusion_test(
    network_tests
    SOURCES
    network_tests.cpp
    LINK_LIBRARIES
    foundation_infrastructure
)
