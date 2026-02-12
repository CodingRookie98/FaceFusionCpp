find_package(nlohmann_json REQUIRED)

add_facefusion_test(shutdown_handler_test
    SOURCES shutdown_handler_test.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(checkpoint_manager_test
    SOURCES checkpoint_manager_test.cpp
    LINK_LIBRARIES services_pipeline test_common
)

add_facefusion_test(metrics_collector_test
    SOURCES metrics_collector_test.cpp
    LINK_LIBRARIES services_pipeline nlohmann_json::nlohmann_json
)
