add_library(foundation_ai)

target_compile_features(foundation_ai PUBLIC cxx_std_20)

set_target_properties(foundation_ai PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF
)

target_sources(foundation_ai
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        inference_session.ixx
        PRIVATE
        inference_session.cpp
)

target_link_libraries(foundation_ai
        PUBLIC
        foundation_infrastructure
        ONNXRuntime::ONNXRuntime
)

if (MSVC)
    target_compile_options(foundation_ai
            PRIVATE
            /permissive-
            /bigobj
            /wd5050
    )
endif ()
