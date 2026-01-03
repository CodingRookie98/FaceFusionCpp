target_sources(${ProgramName} PRIVATE
        FILE_SET CXX_MODULES FILES
        face_landmarker_hub.ixx
        t68By5.ixx
        t2dfan.ixx
        face_landmarker_base.ixx
        peppawutz.ixx
)
target_sources(${ProgramName} PRIVATE
        face_landmarker_hub.cpp
        t68By5.cpp
        t2dfan.cpp
        face_landmarker_base.cpp
        peppawutz.cpp
)

target_include_directories(${ProgramName} PRIVATE
        .
)
