# CMakeLists.txt for app tests

# Config parser tests
add_facefusion_test(config_parser_tests
    SOURCES
        config/config_parser_tests.cpp
    MAIN_LIB
        GTest::gtest
    LINK_LIBRARIES
        config_parser
        GTest::gmock
)

# Config merger tests
add_facefusion_test(config_merger_tests
    SOURCES
        config/config_merger_tests.cpp
    MAIN_LIB
        GTest::gtest
    LINK_LIBRARIES
        config_core
        GTest::gmock
)

# Config validator tests
add_facefusion_test(config_validator_test
    SOURCES
        config/config_validator_test.cpp
    MAIN_LIB
        GTest::gtest_main
    LINK_LIBRARIES
        config_core
        GTest::gmock
)
