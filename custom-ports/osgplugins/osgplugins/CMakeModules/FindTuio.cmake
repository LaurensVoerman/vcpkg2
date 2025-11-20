# Locate Tuio
# This module defines
# TUIO_LIBRARY
# TUIO_FOUND, if false, do not try to link to zlib 
# TUIO_INCLUDE_DIR, where to find the headers
#
# Created by Pjotr Svetachov. 

FIND_PATH(TUIO_INCLUDE_DIR TuioClient.h
    $ENV{TUIO_DIR}/TUIO
    $ENV{TUIO_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
)

FIND_PATH(TUIO_OSC_INCLUDE_DIR osc/OscTypes.h
    $ENV{TUIO_DIR}/oscpack
    $ENV{TUIO_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
)

FIND_LIBRARY(TUIO_LIBRARY 
    NAMES TUIO
    PATHS
    $ENV{TUIO_DIR}/lib
    $ENV{TUIO_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/lib
    /usr/lib
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    /usr/freeware/lib64
)

SET(TUIO_FOUND "NO")
IF(TUIO_LIBRARY AND TUIO_INCLUDE_DIR AND TUIO_OSC_INCLUDE_DIR)
    SET(TUIO_FOUND "YES")
ENDIF(TUIO_LIBRARY AND TUIO_INCLUDE_DIR AND TUIO_OSC_INCLUDE_DIR)
