# - Find HACKRF
# Find the native HACKRF includes and library
# This module defines
#  HACKRF_INCLUDE_DIR, where to find hackrf.h, etc.
#  HACKRF_LIBRARIES, the libraries needed to use HACKRF.
#  HACKRF_FOUND, If false, do not try to use HACKRF.
# also defined, but not for general use are
#  HACKRF_LIBRARY, where to find the HACKRF library.

#MESSAGE("HACKRF_DIR set to ${HACKRF_DIR}" )

FIND_PATH(HACKRF_INCLUDE_DIR hackrf.h
  ${HACKRF_DIR}/include
  /usr/include/libhackrf
)

FIND_LIBRARY(HACKRF_LIBRARY
  NAMES hackrf
  PATHS ${HACKRF_DIR}/libs
  "${HACKRF_DIR}\\win32\\lib"
  /usr/pkgs64/lib
  /usr/lib64
  /usr/lib
  /usr/local/lib
  /usr/lib/x86_64-linux-gnu
  NO_DEFAULT_PATH
)

IF (HACKRF_LIBRARY AND HACKRF_INCLUDE_DIR)
  SET(HACKRF_LIBRARIES ${HACKRF_LIBRARY})
  SET(HACKRF_FOUND "YES")
ELSE (HACKRF_LIBRARY AND HACKRF_INCLUDE_DIR)
  SET(HACKRF_FOUND "NO")
ENDIF (HACKRF_LIBRARY AND HACKRF_INCLUDE_DIR)

IF (HACKRF_FOUND)
  IF (NOT HACKRF_FIND_QUIETLY)
#    MESSAGE(STATUS "Found HACKRF: ${HACKRF_LIBRARIES}")
  ENDIF (NOT HACKRF_FIND_QUIETLY)
ELSE (HACKRF_FOUND)
  IF (HACKRF_FIND_REQUIRED)
#    MESSAGE(FATAL_ERROR "Could not find HACKRF library")
  ENDIF (HACKRF_FIND_REQUIRED)
ENDIF (HACKRF_FOUND)

# Deprecated declarations.
GET_FILENAME_COMPONENT (NATIVE_HACKRF_LIB_PATH ${HACKRF_LIBRARY} PATH)

MARK_AS_ADVANCED(
  HACKRF_LIBRARY
  HACKRF_INCLUDE_DIR
)

