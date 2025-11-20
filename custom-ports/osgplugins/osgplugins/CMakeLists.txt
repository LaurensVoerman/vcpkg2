CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Generate a Config.h file in the build dir
set(Project_type osgRC CACHE STRING "The type of the project you want to build")
set_property(CACHE Project_type PROPERTY STRINGS osgRC Seaports Via_Appia)

#change project name dependent on config - but keep OSGRC_SOURCE_DIR set to find CMakeModules
if(${Project_type} STREQUAL "Seaports")
	set(DSP_EDITION "1")
	PROJECT(DSP)
elseif(${Project_type} STREQUAL "Via_Appia")
	set(VP_EDITION "1")
	PROJECT(Via_Appia)
else()
	PROJECT(OSGRC)
endif()


# We have some custom .cmake scripts not in the official distribution.
# Maybe this can be used override existing behavior if needed?
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
# OpenXR creates a cmake OpenXRConfig.cmake
set(CMAKE_MODULE_PATH "${GLCORE_GLCOREARB_HEADER}/../x64/cmake;${CMAKE_MODULE_PATH}")
set(CMAKE_PREFIX_PATH "${GLCORE_GLCOREARB_HEADER}/../x64/cmake;${CMAKE_PREFIX_PATH}")
if(${Project_type} STREQUAL "Via_Appia")
	FIND_PACKAGE(Soci)
	FIND_PACKAGE(PostgreSQL)
endif()

FIND_PACKAGE(Threads)

# Find OpenGL
FIND_PACKAGE(OpenGL)

IF(UNIX)
    # Not sure what this will do on Cygwin and Msys
    # Also, remember OS X X11 is a user installed option so it may not exist.
    FIND_PACKAGE(X11)
    # Some Unicies need explicit linkage to the Math library or the build fails.
    FIND_LIBRARY(MATH_LIBRARY m)
ENDIF(UNIX)

IF(WIN32)

    IF(MSVC)
        # This option is to enable the /MP switch for Visual Studio 2005 and above compilers
        OPTION(WIN32_USE_MP "Set to ON to build OpenSceneGraph with the /MP option (Visual Studio 2005 and above)." ON)
        MARK_AS_ADVANCED(WIN32_USE_MP)
        IF(WIN32_USE_MP)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        ENDIF(WIN32_USE_MP)

        # More MSVC specific compilation flags
        ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
        ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
        IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
        #compile 64 bit version for WIN7 and up
          ADD_DEFINITIONS(-D_WIN32_WINNT=0x0601)
        ELSE(CMAKE_SIZEOF_VOID_P MATCHES "8")
          ADD_DEFINITIONS(-D_WIN32_WINNT=0x0500)
        ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")

        
    ENDIF(MSVC)

    #needed for net plugin
    SET (OSG_SOCKET_LIBS wsock32)
    # Both Cygwin and Msys need -DNOMINMAX ???
    IF(UNIX)
        ADD_DEFINITIONS(-DNOMINMAX)
    ENDIF(UNIX)
########################################################################################################
# the following options are MSVC specific,
# the first OSG_MSVC_VERSIONED_DLL activate a custom build-time layout that should allow to run examples and application
# fron bin folder without requiring installation step.
# it also prepend "osg${OPENSCENEGRAPH_SOVERSION}-" to only .dll files, leaving .lib files untouched in lib
# it also use a hack to get rid of Debug and Release folder in MSVC projects
# all the .dll and .pdb are in bin and all the .lib and .exp are in lib
#
# the second option disable incremental linking in debug build , that is enabled by default by CMake
##########################################################################################################

    IF(MSVC)
#        OPTION(OSG_MSVC_VERSIONED_DLL "Set to ON to build OpenSceneGraph with versioned dll names" ON)
#        MARK_AS_ADVANCED(OSG_MSVC_VERSIONED_DLL)
        OPTION(OSGRC_MSVC_DEBUG_INCREMENTAL_LINK "Set to OFF to build OpenSceneGraph without incremental linking in debug (release is off by default)" OFF)
        MARK_AS_ADVANCED(OSGRC_MSVC_DEBUG_INCREMENTAL_LINK)
        IF(NOT OSGRC_MSVC_DEBUG_INCREMENTAL_LINK)
            SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
            SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
            SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO")
        ENDIF(NOT OSGRC_MSVC_DEBUG_INCREMENTAL_LINK)
    ENDIF(MSVC)
ENDIF(WIN32)

# Common to all platforms:
SET(OSG_DIR "" CACHE PATH "set to base osg install path")

SET(CMAKE_DEBUG_POSTFIX  "d")
SET(CMAKE_RELWITHDEBINFO_POSTFIX  "rd")

set(OpenSceneGraph_MARK_AS_ADVANCED ON)
find_package(OpenSceneGraph 3.0.0 REQUIRED osgAnimation osgDB osgFX osgGA osgParticle osgShadow osgSim osgTerrain osgText osgUtil osgViewer osgVolume)
OSG_FIND_PATH(OSG_GENERATED_CONFIG osg/Config)
list(APPEND OPENSCENEGRAPH_INCLUDE_DIRS ${OSG_GENERATED_CONFIG_INCLUDE_DIR})
list(REMOVE_DUPLICATES OPENSCENEGRAPH_INCLUDE_DIRS)
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})

IF(DEFINED UNDEFINED_VARIABLE)
  FIND_PACKAGE(FOX)
  FIND_PACKAGE(GLCORE)
  
  FIND_PACKAGE(ZSpace)
  IF(ZSPACE_FOUND)
    IF(NOT DEFINED USE_ZSPACE)
      SET(USE_ZSPACE "YES" CACHE BOOL "Use ZSPACE")
    ENDIF(NOT DEFINED USE_ZSPACE)
  ELSE(ZSPACE_FOUND)
    SET(USE_ZSPACE "NO" CACHE BOOL "Use ZSPACE (ZSPACE api not found)")
  ENDIF(ZSPACE_FOUND)
  
  
  FIND_PACKAGE(FFmpeg)
  IF(FFMPEG_FOUND)
    IF(NOT DEFINED USE_FFMPEG)
      SET(USE_FFMPEG "YES" CACHE BOOL "Use internal osgFFMPEG plugin for syncronized movies")
    ENDIF(NOT DEFINED USE_FFMPEG)
  ELSE(FFMPEG_FOUND)
    SET(USE_FFMPEG "NO" CACHE BOOL "Use internal osgFFMPEG plugin for syncronized movies (FFMPEG not found)")
  ENDIF(FFMPEG_FOUND)
  
  FIND_PACKAGE(Tuio)
  IF(TUIO_FOUND)
    IF(NOT DEFINED USE_TUIO)
      SET(USE_TUIO "YES" CACHE BOOL "Use Tuio for multitouch input")
    ENDIF(NOT DEFINED USE_TUIO)
  ELSE(TUIO_FOUND)
    SET(USE_TUIO "NO" CACHE BOOL "Use Tuio for multitouch input (TUIO not found)")
  ENDIF(TUIO_FOUND)
  
  FIND_PACKAGE(OpenVR)
  IF(OPENVR_SDK_FOUND)
    IF(NOT DEFINED USE_OPENVR)
      SET(USE_OPENVR "YES" CACHE BOOL "Use OpenVR for hmd")
    ENDIF(NOT DEFINED USE_OPENVR)
  ELSE(OPENVR_SDK_FOUND)
    SET(USE_OPENVR "NO" CACHE BOOL "Use OpenVR for hmd (OpenVR not found)")
  ENDIF(OPENVR_SDK_FOUND)
  
  FIND_PACKAGE(OculusSDK )
  IF(OCULUS_SDK_FOUND)
    IF(NOT DEFINED USE_OVR)
      SET(USE_OVR "YES" CACHE BOOL "Use OculusSDK for hmd")
    ENDIF(NOT DEFINED USE_OVR)
  ELSE(OCULUS_SDK_FOUND)
    SET(USE_OVR "NO" CACHE BOOL "Use OculusSDK for hmd (OculusSDK not found)")
  ENDIF(OCULUS_SDK_FOUND)
  
  FIND_PACKAGE(OpenXR )
  IF(OpenXR_FOUND)
    IF(NOT DEFINED USE_OPENXR)
      SET(USE_OPENXR "YES" CACHE BOOL "Use OpenXR for hmd")
    ENDIF(NOT DEFINED USE_OPENXR)
  ELSE(OpenXR_FOUND)
    SET(USE_OPENXR "NO" CACHE BOOL "Use OpenXR for hmd (OpenXR not found)")
  ENDIF(OpenXR_FOUND)
  
  FIND_PACKAGE(OSGAudio)
  IF(OSG_AUDIO_FOUND)
    IF(NOT DEFINED USE_OPENAL)
      SET(USE_OPENAL "YES" CACHE BOOL "Use OSGAudio / OpenAL")
    ENDIF(NOT DEFINED USE_OPENAL)
  ELSE(OSG_AUDIO_FOUND)
    SET(USE_OPENAL "NO" CACHE BOOL "Use OSGAudio / OpenAL (not found)")
  ENDIF(OSG_AUDIO_FOUND)
  
  IF(NOT DEFINED USE_KINECT)
    SET(USE_KINECT "NO" CACHE BOOL "Enable Kinect stuff")
  ENDIF(NOT DEFINED USE_KINECT)
  
ENDIF()
################################################################################
# Create bin and lib directories if required

IF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib)
ENDIF("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")


################################################################################
# Installation stuff
SET(OUTPUT_BINDIR ${PROJECT_BINARY_DIR}/bin/ CACHE PATH "where to put the executables")
MAKE_DIRECTORY(${OUTPUT_BINDIR})

SET(OUTPUT_LIBDIR ${PROJECT_BINARY_DIR}/lib/ CACHE PATH "where to put the libraries")
MAKE_DIRECTORY(${OUTPUT_LIBDIR})

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
IF(WIN32)
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
ELSE(WIN32)
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
ENDIF(WIN32)

FOREACH(CONF ${CMAKE_CONFIGURATION_TYPES})        # For each configuration (Debug, Release, MinSizeRel... and/or anything the user chooses)
    STRING(TOUPPER "${CONF}" CONF)                # Go uppercase (DEBUG, RELEASE...)
    SET("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
    SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
    IF(WIN32)
        SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
    ELSE()
        SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
    ENDIF()
ENDFOREACH()

INCLUDE(OsgMacroUtils)


set(CONFIG_IN_TEXT "CAUTION the Config.h file is automatically generated by Cmake from the src/Config/Config.in file. To edit this file you need to edit Config.in instead.")

SET(OSGRC_CONFIG_HEADER ${PROJECT_BINARY_DIR}/include/Config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/Config/Config.in ${OSGRC_CONFIG_HEADER})

# OSGRC Tools
OPTION(BUILD_TOOLS "Enable to build OSGRC Applications (e.g. mkthumbs, osgconvEx )" OFF)
OPTION(BUILD_PLUGINS "Enable to build OSG Plugins (e.g. ibr skydome)" ON)
# OSGRC Core
include_directories(src ${PROJECT_BINARY_DIR}/include)

ADD_SUBDIRECTORY(src)

IF (BUILD_TOOLS)
    ADD_SUBDIRECTORY(tools)
ENDIF(BUILD_TOOLS)
set_property (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT viewer)