add_modules_library(foundation_infrastructure_test_support STATIC)

target_sources(foundation_infrastructure_test_support
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            test_support.ixx
    PRIVATE
        test_support.cpp
)

target_link_libraries(foundation_infrastructure_test_support
    PUBLIC
        foundation_infrastructure
)
