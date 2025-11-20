INCLUDE(OsgMacroUtils)

SET(TARGET_SRC
    DeprecatedGeometry.cpp
    ConvertToInventor.cpp
    ConvertFromInventor.cpp
    PendulumCallback.cpp
    ReaderWriterIV.cpp
    ShuttleCallback.cpp
)
SET(TARGET_HDRS
    DeprecatedGeometry.h
    ConvertToInventor.h
    ConvertFromInventor.h
    PendulumCallback.h
    ReaderWriterIV.h
    ShuttleCallback.h
)
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
ADD_DEFINITIONS(-DCOIN_NOT_DLL)
else()
ADD_DEFINITIONS(-DCOIN_DLL)
endif()

INCLUDE_DIRECTORIES(${INVENTOR_INCLUDE_DIR})

SET(TARGET_LIBRARIES_VARS INVENTOR_LIBRARY)

SETUP_PLUGIN(iv iv)
