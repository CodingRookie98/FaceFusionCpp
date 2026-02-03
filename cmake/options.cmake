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

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(project_options INTERFACE
            -fuse-ld=lld
        )
    endif()

    target_compile_definitions(project_options INTERFACE
        OPENCV_SKIP_STD_VECTOR_CONTIGUOUS_CHECK
        CV_DO_NOT_INCLUDE_ITERATOR_EXTRAS
        CV_IGNORE_DEPRECATED_STD_VECTOR_ACCESSORS
        CV_DISABLE_S_CAST_HFLOAT
    )
    target_include_directories(project_options INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OpenCV_INCLUDE_DIRS}
    )
endif()
