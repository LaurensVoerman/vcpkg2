# Locate zSpace SDK

# This module searches giflib and defines ZSPACE_LIBRARIES - libraries to
# link to in order to use ZSPACE ZSPACE_FOUND, if false, do not try to link
# ZSPACE_INCLUDE_DIR, where to find the headers ZSPACE_VERSION

if(CMAKE_HOST_WIN32)
  if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "This macro can only be called by a windows host (call to reg.exe")
  endif()

  if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "64")
    set(REG_EXE "C:\\Windows\\Sysnative\\reg.exe")
    SET(LIB_DIR "x64")
    SET(LIB_NAME "zSpaceApi64")
  else()
    set(REG_EXE "reg")
    SET(LIB_DIR "Win32")
    SET(LIB_NAME "zSpaceApi")
  endif()
  
  execute_process(
    COMMAND ${REG_EXE} "query" "HKEY_LOCAL_MACHINE\\SOFTWARE\\zSpace\\zSpace SDK\\Active"
    RESULT_VARIABLE resultzSpaceSDKver
    OUTPUT_VARIABLE zSpaceSDKver
    ERROR_VARIABLE errzSpaceSDKver
    INPUT_FILE NUL
    )
  
  if(${resultzSpaceSDKver} EQUAL 0)	
    string(
  #   REGEX MATCH "    (Default)    REG_SZ    \([0-9.]+\)"
     REGEX MATCH "REG_SZ +\([0-9.]+\)"
        zSpace_versions_regex ${zSpaceSDKver})
      set(zSpace_current_version ${CMAKE_MATCH_1})
  else()
    message ("reg query error: ${resultzSpaceSDKver} ; ${errzSpaceSDKver}" )
  endif()
  
  
  #message ( "Found zSpace sdk version ${zSpace_current_version}" ---)
  
  if(NOT zSpace_current_version)
    set(zSpace_current_version "3.0.0.318")
    set(zSpace_current_version "4.0.0.3")
    message ( "Fallback to default zSpaceSDKver ${zSpace_current_version}" )
  endif()
  
  #only keep ${zSpace_current_version}
  unset(resultzSpaceSDKver)
  unset(zSpaceSDKver)
  unset(errzSpaceSDKver)
  unset(zSpace_versions_regex)
  
  #message ("COMMAND C:\\Windows\\Sysnative\\reg.exe query \"HKEY_LOCAL_MACHINE\\SOFTWARE\\zSpace\\zSpace SDK\\${zSpace_current_version}\" /f Install Path" )
  
  execute_process(
    COMMAND ${REG_EXE} "query" "HKEY_LOCAL_MACHINE\\SOFTWARE\\zSpace\\zSpace SDK\\${zSpace_current_version}" "/f" "Install Path"
    RESULT_VARIABLE resultzSpaceSDKpath
    OUTPUT_VARIABLE zSpaceSDKpath
    ERROR_VARIABLE errzSpaceSDKpath
    INPUT_FILE NUL
    )
  
  if(${resultzSpaceSDKpath} EQUAL 0)	
    string(
     REGEX MATCH "    Install Path    REG_SZ    \(.+\)"
        zSpace_versions_regex ${zSpaceSDKpath})
      set(zSpace_current_path ${CMAKE_MATCH_1}) 
  else()
    message ("reg query error: ${resultzSpaceSDKpath} ; ${errzSpaceSDKpath}" )
  endif()
  
  #only keep ${zSpace_current_path}
  unset(resultzSpaceSDKpath)
  unset(zSpaceSDKpath)
  unset(errzSpaceSDKpath)
  unset(zSpace_versions_regex)
  unset(REG_EXE)
else()
#*NIX
# total guess: I dont know about a non windows version of the zSpace SDK
    SET(LIB_DIR "")
    SET(LIB_NAME "zSpaceApi")
    set(zSpace_current_version "3.0.0.318")
    set(zSpace_current_path "/opt/zSpaceSDK/${zSpace_current_version}")
endif()

if(NOT zSpace_current_path)
  set(zSpace_current_path C:/zSpace/zSpaceSDKs/${zSpace_current_version})
  message ( "Using zSpace sdk path ${zSpace_current_path}" )
#else()
#  message ( "Found zSpace sdk path ${zSpace_current_path}" )
endif()


FIND_PATH(ZSPACE_INCLUDE_DIR zSpace.h
    PATHS
    ${zSpace_current_path}/Inc
)

MACRO(FIND_ZSPACE_LIBRARY MYLIBRARY MYLIBRARYNAME)

    FIND_LIBRARY(${MYLIBRARY}
        NAMES ${MYLIBRARYNAME}
        PATHS
        ${zSpace_current_path}/Lib/${LIB_DIR}
    )
ENDMACRO(FIND_ZSPACE_LIBRARY LIBRARY LIBRARYNAME)

FIND_ZSPACE_LIBRARY(ZSPACE_LIBRARY ${LIB_NAME})


unset(LIB_DIR)
unset(LIB_NAME)

SET(ZSPACE_FOUND "NO")
IF(ZSPACE_LIBRARY AND ZSPACE_INCLUDE_DIR)
    SET(ZSPACE_FOUND "YES")
ENDIF(ZSPACE_LIBRARY AND ZSPACE_INCLUDE_DIR)
