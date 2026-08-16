# CMakeLists.txt for app tests

# Config parser tests
add_facefusion_test(config_parser_tests
    SOURCES
        config/config_parser_test.cpp
    MAIN_LIB
        GTest::gtest
    LINK_LIBRARIES
        config_parser
        GTest::gmock
)

# Config merger tests
add_facefusion_test(config_merger_tests
    SOURCES
        config/config_merger_test.cpp
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
# Web server health payload tests
add_facefusion_test(web_server_health_test
    SOURCES
        web_server_health_test.cpp
    MAIN_LIB
        GTest::gtest_main
    LINK_LIBRARIES
        app_web
        app_version
)
target_compile_features(web_server_health_test PRIVATE cxx_std_20)
# Web task manager tests
add_facefusion_test(web_task_manager_test
    SOURCES
        web/task_manager_test.cpp
    MAIN_LIB
        GTest::gtest_main
    LINK_LIBRARIES
        app_web
        config_core
        services_pipeline
        foundation_infrastructure
)
target_compile_features(web_task_manager_test PRIVATE cxx_std_20)
