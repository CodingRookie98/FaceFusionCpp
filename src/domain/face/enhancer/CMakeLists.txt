
target_sources(domain_face PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        face_enhancer.ixx
        face_enhancer_api.ixx
        face_enhancer_types.ixx
        face_enhancer_factory.ixx
)

target_sources(domain_face PRIVATE
    FILE_SET cxx_modules_private TYPE CXX_MODULES FILES
        impl/face_enhancer_impl_base.ixx
        impl/code_former.ixx
        impl/gfp_gan.ixx
)

target_sources(domain_face PRIVATE
    face_enhancer_factory.cpp
    impl/code_former.cpp
    impl/gfp_gan.cpp
)
