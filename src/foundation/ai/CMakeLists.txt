add_modules_library(foundation_ai)

target_sources(foundation_ai
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        inference_session.ixx
        inference_session_registry.ixx
        session_pool.ixx
        PRIVATE
        inference_session.cpp
        inference_session_registry.cpp
        session_pool.cpp
)

target_link_libraries(foundation_ai
        PUBLIC
        foundation_infrastructure
        ONNXRuntime::ONNXRuntime
)
