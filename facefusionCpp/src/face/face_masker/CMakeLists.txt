target_sources(${app_facefusioncpp} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_masker_base.ixx
        region.ixx
        occlusion.ixx
        face_masker_hub.ixx
)
target_sources(${app_facefusioncpp} PRIVATE
        face_masker_base.cpp
        region.cpp
        occlusion.cpp
        face_masker_hub.cpp
)

target_include_directories(${app_facefusioncpp} PRIVATE
        .
)
