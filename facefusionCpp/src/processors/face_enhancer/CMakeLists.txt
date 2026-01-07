target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/gfp_gan.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/code_former.ixx
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/face_enhancer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gfp_gan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/code_former.cpp
)

target_include_directories(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
