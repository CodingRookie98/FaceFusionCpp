target_sources(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer_base.h
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/expression_restorer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/live_portrait_helper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/live_portrait_helper.h
)
target_include_directories(${ProgramName} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
