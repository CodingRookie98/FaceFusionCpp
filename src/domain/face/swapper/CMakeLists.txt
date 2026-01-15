add_modules_library(domain_face_swapper)

find_package(ONNX REQUIRED)
find_package(protobuf REQUIRED)
find_package(thread-pool CONFIG REQUIRED)

target_sources(domain_face_swapper
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
            face_swapper.ixx
            face_swapper_api.ixx
            face_swapper_types.ixx
            face_swapper_factory.ixx
            impl/swapper_impl_base.ixx
            impl/inswapper.ixx
            impl/mask_compositor.ixx
    PRIVATE
            impl/inswapper.cpp
)


target_link_libraries(domain_face_swapper
    PUBLIC
        foundation_ai
        domain_face
        domain_face_masker
        ${OpenCV_LIBS}
    PRIVATE
        ONNXRuntime::ONNXRuntime
        ONNX::onnx
        ONNX::onnx_proto
        protobuf::libprotobuf
        dp::thread-pool
)

target_compile_definitions(domain_face_swapper PRIVATE ONNX_ML)
