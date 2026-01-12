add_modules_library(domain_common)

target_sources(domain_common
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            common.ixx
            common_types.ixx
)
