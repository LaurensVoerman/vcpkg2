# - Find OpenVR SDK
# Find the OpenVR SDK headers and libraries.
#
#  OPENVR_SDK_INCLUDE_DIRS - where to find openvr.h, etc.
#  OPENVR_SDK_LIBRARIES    - List of libraries when using OpenVR SDK.
#  OPENVR_SDK_FOUND        - True if OpenVR SDK found.

IF (DEFINED ENV{OPENVR_SDK_ROOT_DIR})
    SET(OPENVR_SDK_ROOT_DIR "$ENV{OPENVR_SDK_ROOT_DIR}")
ENDIF()
SET(OPENVR_SDK_ROOT_DIR
    "${OPENVR_SDK_ROOT_DIR}"
    CACHE
    PATH
    "Root directory to search for OpenVR SDK")

# Look for the header file.
FIND_PATH(OPENVR_SDK_INCLUDE_DIRS NAMES openvr.h HINTS 
	${OPENVR_SDK_ROOT_DIR}/headers )

# Determine architecture
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
	IF(MSVC)
		SET(_OPENVR_SDK_LIB_ARCH "win64")
	ENDIF()
ELSE()
	IF(MSVC)
		SET(_OPENVR_SDK_LIB_ARCH "win32")
	ENDIF()
ENDIF()

MARK_AS_ADVANCED(_OPENVR_SDK_LIB_ARCH)

# Append "d" to debug libs on windows platform
IF (WIN32)
	SET(CMAKE_DEBUG_POSTFIX d)
ENDIF()


# Locate OpenVR license file
SET(_OPENVR_SDK_LICENSE_FILE "${OPENVR_SDK_ROOT_DIR}/LICENSE")
IF(EXISTS "${_OPENVR_SDK_LICENSE_FILE}") 
	SET(OPENVR_SDK_LICENSE_FILE "${_OPENVR_SDK_LICENSE_FILE}" CACHE INTERNAL "The location of the OpenVR SDK license file")
ENDIF()

# Look for the library.
FIND_LIBRARY(OPENVR_SDK_LIBRARY NAMES openvr_api HINTS ${OPENVR_SDK_ROOT_DIR} 
                                                      ${OPENVR_SDK_ROOT_DIR}/lib/${_OPENVR_SDK_LIB_ARCH}
                                                    )
    
MARK_AS_ADVANCED(OPENVR_SDK_LIBRARY)
MARK_AS_ADVANCED(OPENVR_SDK_LIBRARY_DEBUG)

# No debug library for OpenVR
SET(OPENVR_SDK_LIBRARY_DEBUG ${OPENVR_SDK_LIBRARY})

SET(OPENVR_SDK_LIBRARIES optimized ${OPENVR_SDK_LIBRARY} debug ${OPENVR_SDK_LIBRARY_DEBUG})

# handle the QUIETLY and REQUIRED arguments and set OPENVR_SDK_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENVR_SDK DEFAULT_MSG OPENVR_SDK_LIBRARIES OPENVR_SDK_INCLUDE_DIRS)

MARK_AS_ADVANCED(OPENVR_SDK_LIBRARIES OPENVR_SDK_INCLUDE_DIRS)
