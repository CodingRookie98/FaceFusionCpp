function(add_cxx_module_library target)
    add_library(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)

    if (MSVC)
        target_compile_options(${target} PRIVATE /permissive-)
    endif()
endfunction()
