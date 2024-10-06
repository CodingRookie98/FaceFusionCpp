target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/frame_enhancer_helper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/real_esr_gan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/real_esr_gan.h
        ${CMAKE_CURRENT_SOURCE_DIR}/real_hat_gan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/real_hat_gan.h
)

target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
