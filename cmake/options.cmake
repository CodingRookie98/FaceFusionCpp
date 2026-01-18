add_library(project_options INTERFACE)

target_compile_features(project_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(project_options INTERFACE
        /W4
        /permissive-
        /utf-8
        /EHsc
        /bigobj
        /wd5050 # Header unit warning
    )
    target_compile_definitions(project_options INTERFACE
        _WIN32_WINNT=0x0A00
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    target_compile_options(project_options INTERFACE
        -Wall -Wextra -Wpedantic
    )
endif()
