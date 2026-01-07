target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.ixx
)
target_sources(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_analyser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_selector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.cpp
)
target_include_directories(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(face_detector)
add_subdirectory(face_landmarker)
add_subdirectory(face_recognizer)
add_subdirectory(face_masker)
add_subdirectory(face_classifier)
