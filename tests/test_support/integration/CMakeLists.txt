add_library(integration_test_support STATIC
    ../../integration/common/global_test_environment.cpp
)

target_link_libraries(integration_test_support
    PUBLIC
        GTest::gtest
        domain_face
        domain_face_analyser
        foundation_ai
)

target_include_directories(integration_test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
