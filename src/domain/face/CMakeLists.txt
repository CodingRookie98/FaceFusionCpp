find_package(OpenCV REQUIRED)

add_modules_library(domain_face)

target_sources(domain_face
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_types.ixx
            face.ixx
            face_test_support.ixx
    PRIVATE
        face.cpp
)

target_link_libraries(domain_face
    PUBLIC
        ${OpenCV_LIBS}
        domain_common
)
