target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.ixx
)
target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_store.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_helper.cpp
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(face_analyser)
