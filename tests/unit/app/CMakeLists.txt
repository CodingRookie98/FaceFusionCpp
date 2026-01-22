# CMakeLists.txt for app tests

# Config parser tests
add_facefusion_test(config_parser_tests
    SOURCES
        config/config_parser_tests.cpp
    LINK_LIBRARIES
        config_parser
        GTest::gmock
)
