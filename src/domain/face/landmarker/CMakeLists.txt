target_sources(domain_face
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_landmarker_api.ixx
            face_landmarker_factory.ixx
            face_landmarker.ixx
    PRIVATE
        face_landmarker_factory.cpp
)

add_subdirectory(impl)
