target_sources(${target_app_facefusioncpp}
        PRIVATE FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/real_esr_gan.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/real_hat_gan.ixx
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/real_esr_gan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/real_hat_gan.cpp
)

target_include_directories(${target_app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
