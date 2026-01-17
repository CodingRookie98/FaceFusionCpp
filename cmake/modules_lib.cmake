function(add_modules_library TARGET_NAME)
    add_library(${TARGET_NAME} ${ARGN}) # Support STATIC/SHARED arguments

    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)

    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF
    )

    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /permissive-
            /bigobj
            /wd5050
        )
    endif()
    setup_resource_copying(${TARGET_NAME})
endfunction()
