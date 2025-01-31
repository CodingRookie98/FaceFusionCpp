target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer_base.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/live_portrait.ixx
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/live_portrait.cpp
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
