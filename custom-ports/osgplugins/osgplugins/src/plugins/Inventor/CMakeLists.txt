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

#COIN uses #pragma comment link autolinking
#SET(TARGET_LIBRARIES_VARS INVENTOR_LIBRARY)

set(TARGET_NAME iv)
set(DYNAMIC_OPENSCENEGRAPH 1)

add_definitions(-D_USRDLL)


string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)

cmake_path(GET INVENTOR_LIBRARY_${CMAKE_BUILD_TYPE_UPPER} PARENT_PATH CURRENT_LIB_DIR)
link_directories(${CURRENT_LIB_DIR})
MESSAGE("link_directories(${CURRENT_LIB_DIR})")


SETUP_PLUGIN(${TARGET_NAME})
target_link_libraries(${TARGET_TARGETNAME} ${OPENSCENEGRAPH_LIBRARIES})
#osg by line above
#target_link_libraries(${TARGET_TARGETNAME} osgd.lib osgDBd.lib osgUtild.lib )
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
  #link dependencies of coin lib
  #both dbg and rel - link dir changes
  target_link_libraries(${TARGET_TARGETNAME} sndfile.lib libmp3lame-static.lib mpg123.lib libmpghip-static.lib opus.lib vorbis.lib vorbisfile.lib ogg.lib FLAC.lib gdiplus.lib Shlwapi.lib)
  IF(${CMAKE_BUILD_TYPE_UPPER} STREQUAL "RELEASE")
  target_link_libraries(${TARGET_TARGETNAME} zlib.lib simage1s.lib libexpatMD.lib)
  ELSEIF(${CMAKE_BUILD_TYPE_UPPER} STREQUAL "DEBUG")
  target_link_libraries(${TARGET_TARGETNAME} zlibd.lib simage1sd.lib libexpatdMD.lib)
  ENDIF()
endif()
SET_PROPERTY(TARGET ${TARGET_TARGETNAME} PROPERTY FOLDER "Plugins")
