# - Find ITPP
# Find the native ITPP includes and library
# This module defines
#  ITPP_INCLUDE_DIR, where to find itpp/itbase.h, etc.
#  ITPP_LIBRARIES, the libraries needed to use ITPP.
#  ITPP_FOUND, If false, do not try to use ITPP.
# also defined, but not for general use are
#  ITPP_LIBRARY, where to find the ITPP library.

#MESSAGE("ITPP_DIR set to ${ITPP_DIR}" )

FIND_PATH(ITPP_INCLUDE_DIR itpp/itbase.h
  ${ITPP_DIR}/include
  /usr/pkgs64/include
  /usr/include
  /usr/local/include
)

FIND_LIBRARY(ITPP_LIBRARY_NORMAL
  NAMES itpp
  PATHS ${ITPP_DIR}/libs
  "${ITPP_DIR}\\win32\\lib"
  /usr/pkgs64/lib
  /usr/lib64
  /usr/lib/x86_64-linux-gnu
  /usr/lib/aarch64-linux-gnu
  /usr/lib
  /usr/local/lib
  NO_DEFAULT_PATH
)

FIND_LIBRARY(ITPP_LIBRARY_DEBUG
  NAMES itpp_debug
  PATHS ${ITPP_DIR}/libs
  "${ITPP_DIR}\\win32\\lib"
  /usr/pkgs64/lib
  /usr/lib64
  /usr/lib
  /usr/local/lib
  NO_DEFAULT_PATH
)

IF (ITPP_LIBRARY_NORMAL OR ITPP_LIBRARY_DEBUG)
  IF (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ITPP_LIBRARY_DEBUG)
    SET (ITPP_LIBRARY ${ITPP_LIBRARY_DEBUG})
  ELSE (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ITPP_LIBRARY_DEBUG)
    SET (ITPP_LIBRARY ${ITPP_LIBRARY_NORMAL})
  ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ITPP_LIBRARY_DEBUG)
ENDIF (ITPP_LIBRARY_NORMAL OR ITPP_LIBRARY_DEBUG)

IF (ITPP_LIBRARY AND ITPP_INCLUDE_DIR)
  SET(ITPP_LIBRARIES ${ITPP_LIBRARY})
  SET(ITPP_FOUND "YES")
ELSE (ITPP_LIBRARY AND ITPP_INCLUDE_DIR)
  SET(ITPP_FOUND "NO")
ENDIF (ITPP_LIBRARY AND ITPP_INCLUDE_DIR)

IF (ITPP_FOUND)
  IF (NOT ITPP_FIND_QUIETLY)
    MESSAGE(STATUS "Found ITPP: ${ITPP_LIBRARIES}")
  ENDIF (NOT ITPP_FIND_QUIETLY)
ELSE (ITPP_FOUND)
  IF (ITPP_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find ITPP library")
  ENDIF (ITPP_FIND_REQUIRED)
ENDIF (ITPP_FOUND)

# Deprecated declarations.
GET_FILENAME_COMPONENT (NATIVE_ITPP_LIB_PATH ${ITPP_LIBRARY} PATH)

MARK_AS_ADVANCED(
  ITPP_LIBRARY
  ITPP_INCLUDE_DIR
)

