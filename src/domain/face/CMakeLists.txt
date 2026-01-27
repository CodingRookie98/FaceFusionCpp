find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)

add_modules_library(domain_face)

target_sources(domain_face
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES
            face_types.ixx
            face.ixx
            face_helper.ixx
            face_selector.ixx
            face_store.ixx
    PRIVATE
        face.cpp
        face_helper.cpp
        face_selector.cpp
        face_store.cpp
)


target_link_libraries(domain_face
    PUBLIC
        ${OpenCV_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        domain_common
        foundation_ai
        domain_ai
)


add_subdirectory(detector)
add_subdirectory(classifier)
add_subdirectory(landmarker)
add_subdirectory(recognizer)
add_subdirectory(masker)
add_subdirectory(analyser)
add_subdirectory(swapper)
add_subdirectory(enhancer)
add_subdirectory(expression)





