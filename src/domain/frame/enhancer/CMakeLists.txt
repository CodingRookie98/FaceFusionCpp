target_sources(domain_frame PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        frame_enhancer.ixx
        frame_enhancer_api.ixx
        frame_enhancer_types.ixx
        frame_enhancer_factory.ixx
)

target_sources(domain_frame PRIVATE
    FILE_SET cxx_modules_private TYPE CXX_MODULES FILES
        impl/frame_enhancer_impl.ixx
)

target_sources(domain_frame PRIVATE
    frame_enhancer_factory.cpp
    impl/frame_enhancer_impl.cpp
)
