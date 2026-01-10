target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_core.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_model.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_options.ixx
        ${CMAKE_CURRENT_SOURCE_DIR}/serialize_task.ixx
)



target_include_directories(${app_facefusioncpp} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)