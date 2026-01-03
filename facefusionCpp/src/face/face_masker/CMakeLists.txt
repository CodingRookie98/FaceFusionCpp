target_sources(${target_app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_masker_base.ixx
        region.ixx
        occlusion.ixx
        face_masker_hub.ixx
)
target_sources(${target_app_facefusioncpp} PRIVATE
        face_masker_base.cpp
        region.cpp
        occlusion.cpp
        face_masker_hub.cpp
)

target_include_directories(${target_app_facefusioncpp} PRIVATE
        .
)
