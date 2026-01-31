find_package(nlohmann_json REQUIRED)

add_facefusion_test(test_shutdown_handler
    SOURCES test_shutdown_handler.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(test_checkpoint_manager
    SOURCES test_checkpoint_manager.cpp
    LINK_LIBRARIES services_pipeline
)

add_facefusion_test(test_metrics_collector
    SOURCES test_metrics_collector.cpp
    LINK_LIBRARIES services_pipeline nlohmann_json::nlohmann_json
)
