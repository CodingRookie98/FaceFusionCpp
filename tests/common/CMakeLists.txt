add_library(test_common STATIC)

target_sources(test_common
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            fixtures/base_test_fixture.ixx
            fixtures/unit_test_fixture.ixx
            fixtures/integration_test_fixture.ixx
            matchers/opencv_matchers.ixx
            generators/image_generator.ixx
            generators/face_generator.ixx
    PRIVATE
        fixtures/base_test_fixture.cpp
        fixtures/unit_test_fixture.cpp
        fixtures/integration_test_fixture.cpp
        fixtures/global_env_helper.cpp
        test_paths.cpp
)

target_link_libraries(test_common
    PUBLIC
        GTest::gtest
        GTest::gmock
        ${OpenCV_LIBS}
        domain_face
    PRIVATE
)

target_compile_features(test_common PUBLIC cxx_std_20)

target_include_directories(test_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
