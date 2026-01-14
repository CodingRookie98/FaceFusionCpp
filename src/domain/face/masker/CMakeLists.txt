add_modules_library(domain_face_masker)

target_sources(domain_face_masker
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            face_masker.ixx
            face_masker_api.ixx
            face_masker_factory.ixx
            impl/occlusion_masker.ixx
            impl/region_masker.ixx
    PRIVATE
        face_masker_factory.cpp
        impl/occlusion_masker.cpp
        impl/region_masker.cpp
)

target_link_libraries(domain_face_masker
    PUBLIC
        domain_face
        foundation_ai
        ${OpenCV_LIBS}
)
