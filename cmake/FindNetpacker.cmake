#[=======================================================================[.rst:
FindNetpacker
-------------

Locate the Netpacker library

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``netpacker::netpacker``
  The Netpacker library, if found

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``NETPACKER_INCLUDE_DIRS``
  where to find netpacker.h
``NETPACKER_FOUND``
  if false, do not try to link to Netpacker
``NETPACKER_VERSION``
  the human-readable string containing the version of Netpacker if found
``NETPACKER_VERSION_MAJOR``
  Netpacker major version
``NETPACKER_VERSION_MINOR``
  Netpacker minor version
``NETPACKER_VERSION_PATCH``
  Netpacker patch version

Cache variables
^^^^^^^^^^^^^^^

These variables may optionally be set to help this module find the correct files:

``NETPACKER_INCLUDE_DIR``
  where to find netpacker.h

#]=======================================================================]
#

# NETPACKER_INCLUDE_DIR
# NETPACKER_FOUND        - True if netpacker was found
# NETPACKER_INCLUDE_DIRS - Path to netpacker include directory

find_path(NETPACKER_INCLUDE_DIR
  NAMES netpacker/netpacker.h
)

if(NETPACKER_INCLUDE_DIR AND EXISTS "${NETPACKER_INCLUDE_DIR}/netpacker/version.h")
  file(STRINGS "${NETPACKER_INCLUDE_DIR}/netpacker/version.h" _version_line
       REGEX "#define[ \t]+NETPACKER_VERSION_[A-Z]+[ \t]+[0-9]+")
  if(_version_line)
    string(REGEX MATCH "NETPACKER_VERSION_MAJOR[ \t]+([0-9]+)" _major "${_version_line}")
    set(_major ${CMAKE_MATCH_1})
    string(REGEX MATCH "NETPACKER_VERSION_MINOR[ \t]+([0-9]+)" _minor "${_version_line}")
    set(_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "NETPACKER_VERSION_PATCH[ \t]+([0-9]+)" _patch "${_version_line}")
    set(_patch ${CMAKE_MATCH_1})
    set(NETPACKER_VERSION "${_major}.${_minor}.${_patch}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Netpacker
  FOUND_VAR NETPACKER_FOUND
  REQUIRED_VARS NETPACKER_INCLUDE_DIR
  VERSION_VAR NETPACKER_VERSION
)

if(NETPACKER_FOUND)
  set(NETPACKER_INCLUDE_DIRS "${NETPACKER_INCLUDE_DIR}")
  set(NETPACKER_LIBRARIES netpacker::netpacker)
  if(NOT TARGET netpacker::netpacker)
    add_library(netpacker::netpacker INTERFACE IMPORTED)
    target_include_directories(netpacker::netpacker INTERFACE ${NETPACKER_INCLUDE_DIR})
  endif()
else()
  message(FATAL_ERROR "Could not find Netpacker")
endif()

mark_as_advanced(NETPACKER_INCLUDE_DIR)
