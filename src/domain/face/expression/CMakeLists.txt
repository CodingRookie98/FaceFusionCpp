set(module_name domain_face_expression)

add_library(${module_name})

target_sources(${module_name}
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            face_expression.ixx
            face_expression_api.ixx
            face_expression_types.ixx
            face_expression_factory.ixx
            impl/live_portrait.ixx
    PRIVATE
            impl/live_portrait.cpp
)

target_link_libraries(${module_name}
    PUBLIC
        domain_face
        foundation_ai
        opencv_core
        opencv_imgproc
        domain_face_masker
    PRIVATE
        # domain_face_helper # Included in domain_face
        ONNXRuntime::ONNXRuntime # If needed for symbols
)
