# ----------------------------------------------------------------------------
# Copyright (C) 2014 Verizon.  All Rights Reserved.
# All Rights Reserved
#
#   Author: Reed P Morrison
#   Date:   02/07/2014
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# ------------------------------------------------------------------------------

# Project setup
project(waflz)
cmake_minimum_required(VERSION 2.8.0)

# Build options
option(GCC_OPTIONS       "Command line options passed to gcc or 'native' to compile for this hardware" OFF)
option(FORTIFY           "Fortify Source GCC options" OFF)
option(DEBUG_MODE        "Compile in debug mode." OFF)
option(BUILD_TCMALLOC    "Build with tcmalloc" OFF)
option(BUILD_PROFILER    "Enable google cpu and heap profiler support" OFF)
option(BUILD_SYMBOLS     "Build with Symbols" OFF)
option(BUILD_ASAN        "Build with Address Sanitizer" OFF)
option(BUILD_UBSAN       "Build with Undefined Behavior Sanitizer" OFF)

# ------------------------------------------------------------------------------
# Compiler options
# ------------------------------------------------------------------------------
add_definitions(-std=c++0x)
add_definitions(-Wall -Werror -Weffc++ -Wformat -Wformat-security)
#add_definitions(-pedantic)

# ------------------------------------------------------------------------------
# Display the current settings
# ------------------------------------------------------------------------------
message(STATUS "Build Configuration:")
message("")
message("    Build Option                               Variable                Value                 ")
message("    -----------------------------------------------------------------------------------------")
message("    Debug mode:                                " "DEBUG_MODE              " ${DEBUG_MODE})
message("    Fortify Source:                            " "FORTIFY                 " ${FORTIFY})
message("    Install path:                              " "INSTALL_PREFIX          " ${CMAKE_INSTALL_PREFIX})
message("    Enable google cpu/heap profiler support:   " "BUILD_PROFILER          " ${BUILD_PROFILER})
message("    Build Symbols                              " "BUILD_SYMBOLS           " ${BUILD_SYMBOLS})
message("    Build with tcmalloc:                       " "BUILD_TCMALLOC          " ${BUILD_TCMALLOC})
message("    Build with Address Sanitizer:              " "BUILD_ASAN              " ${BUILD_ASAN})
message("    Build with Undefined Behavior Sanitizer:   " "BUILD_UBSAN             " ${BUILD_UBSAN})
message("")

if (FORTIFY)
  add_definitions(-D_FORTIFY_SOURCE=2 -O1 -Wl,-z,relro,-z,now)
endif()

# ------------------------------------------------------------------------------
# ASAN
# ------------------------------------------------------------------------------
if (BUILD_ASAN)
  set(LIBRARIES ${LIBRARIES} asan)
  add_definitions(-g3 -fno-omit-frame-pointer -fsanitize=address)
  set(DEBUG_MODE ON)
  set(BUILD_PROFILER OFF)
  set(BUILD_TCMALLOC OFF)
# ------------------------------------------------------------------------------
# UBSAN
# ------------------------------------------------------------------------------
elseif (BUILD_UBSAN)
  set(LIBRARIES ${LIBRARIES} ubsan)
  add_definitions(-g3 -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover)
  set(DEBUG_MODE ON)
  set(BUILD_PROFILER OFF)
  set(BUILD_TCMALLOC OFF)
endif()

# ------------------------------------------------------------------------------
# Libraries options
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Build PROFILER
# ------------------------------------------------------------------------------
if (BUILD_PROFILER)
  add_definitions(-DENABLE_PROFILER=1)
  if (BUILD_UBUNTU)
    find_ubuntu_package(LIBPROFILER google/profiler.h libprofiler.a google-perftools libgoogle-perftools-dev)
  else()
    set(LIBRARIES ${LIBRARIES} tcmalloc profiler)
  endif()
endif()

# ------------------------------------------------------------------------------
# Build TCMALLOC
# ------------------------------------------------------------------------------
if (BUILD_TCMALLOC)
  if (BUILD_UBUNTU)
    find_ubuntu_package(LIBTCMALLOC google/tcmalloc.h libtcmalloc.a "" libgoogle-perftools-dev)
    find_ubuntu_package(LIBZMA lzma.h liblzma.a liblzma5 liblzma-dev)
    find_ubuntu_package(LIBUNWIND libunwind.h libunwind.a "" libunwind8-dev)
    LIST(APPEND LIBRARIES pthread)  # yes it's wierd as shit to have to include this again...
  else()
    set(LIBRARIES ${LIBRARIES} tcmalloc)
  endif()
endif()

# ------------------------------------------------------------------------------
# Optional flags
# ------------------------------------------------------------------------------
if (DEBUG_MODE)
  add_definitions(-O0 -g3)
else()
  add_definitions(-O2)
endif()
if (BUILD_SYMBOLS)
  add_definitions(-g3)
endif()

# include source and test directories
add_subdirectory(proto)
add_subdirectory(src)

# ------------------------------------------------
# Debian Package Support
# ------------------------------------------------
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_VERSION "1.1.0")
SET(CPACK_PACKAGE_FILE_NAME "waflz-${CPACK_DEBIAN_PACKAGE_VERSION}-Linux")
SET(CPACK_DEBIAN_PACKAGE_NAME "waflz")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Reed Morrison")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Basic ModSecurity Implementation.")
SET(CPACK_PACKAGE_DESCRIPTION "Basic ModSecurity Implementation for Verizon Digital.")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "google-perftools")

# cpack doesn't actually use the following, but the build.sh command does
SET(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "libgoogle-perftools-dev, libjsoncpp-dev, libprotobuf-dev")

SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
INCLUDE(CPack)
