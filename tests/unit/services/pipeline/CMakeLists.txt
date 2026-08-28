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

add_facefusion_test(utils_test
    SOURCES utils_test.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(domain_service_cache_test
    SOURCES domain_service_cache_test.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(strict_queue_capacity_test
    SOURCES strict_queue_capacity_test.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(checkpoint_throttle_test
    SOURCES checkpoint_throttle_test.cpp
    LINK_LIBRARIES services_pipeline
)
