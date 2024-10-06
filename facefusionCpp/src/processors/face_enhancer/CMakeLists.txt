target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_helper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_gfpgan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_gfpgan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/code_former.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/code_former.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
