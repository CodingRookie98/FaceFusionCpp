target_sources(domain_face
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            landmarker_impl.ixx
    PRIVATE
        t2dfan.cpp
        peppawutz.cpp
        t68by5.cpp
)
