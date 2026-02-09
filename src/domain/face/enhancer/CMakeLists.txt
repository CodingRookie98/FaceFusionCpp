add_modules_library(domain_face_enhancer)

target_sources(domain_face_enhancer PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        face_enhancer.ixx
        face_enhancer_api.ixx
        face_enhancer_types.ixx
        face_enhancer_factory.ixx
)

target_sources(domain_face_enhancer PRIVATE
    FILE_SET cxx_modules_private TYPE CXX_MODULES FILES
        impl/face_enhancer_impl_base.ixx
        impl/code_former.ixx
        impl/gfp_gan.ixx
)

target_sources(domain_face_enhancer PRIVATE
    face_enhancer_factory.cpp
    impl/code_former.cpp
    impl/gfp_gan.cpp
)

target_link_libraries(domain_face_enhancer
    PUBLIC
        domain_face
        foundation_ai
        ${OpenCV_LIBS}
)
