# ----------------------------------------------------------------------------
# Copyright (C) 2015 Verizon.  All Rights Reserved.
# All Rights Reserved
#
#   Author: Reed P Morrison
#   Date:   09/30/2015  
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
cmake_minimum_required(VERSION 2.6.0)

# include paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../../proto
    ./ndebug
)

# Source files
SET(SRCS
    ./ndebug/ndebug.cc
    ./waflz/wafl.cc
    ./waflz/wafl_parser.cc
    ./waflz/wafl_filter.cc
    ./util/util.cc
    ./util/uri.cc
    ./util/http_request.cc
    ./json_protobuf/json_protobuf.cc
    ${CMAKE_BINARY_DIR}/proto/waflz.pb.cc
)

# append -Wno-effc++ to the protobuf-generated source files.  As appended, overrides previous -Weffc++
set_property(SOURCE ${CMAKE_BINARY_DIR}/proto/waflz.pb.cc APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-effc++")

# Headers
SET(HDRS
    ./ndebug/ndebug.h
    ./waflz/wafl.h
    ./waflz/wafl_parser.h
    ./waflz/wafl_filter.h
    ./util/util.h
    ./util/http_request.h
    ./json_protobuf/json_protobuf.h
    ${CMAKE_BINARY_DIR}/proto/waflz.pb.h
)

# Tell the build system protobuf files are automatically generated
set_source_files_properties(
    ${CMAKE_BINARY_DIR}/proto/waflz.pb.h ${CMAKE_BINARY_DIR}/proto/waflz.pb.cc
    PROPERTIES GENERATED TRUE
)

add_library(waflzcore STATIC ${HDRS} ${SRCS})

# Create symlink for libwaflzcore.so.<soversion>
SET_TARGET_PROPERTIES(waflzcore
  PROPERTIES
  SOVERSION 0
)

# Add install target for the library
install(TARGETS waflzcore
  ARCHIVE DESTINATION lib
  COMPONENT Libraries
)

# Add install target for header files
#install(FILES ${HDRS}
#  DESTINATION include/waflz
#  COMPONENT Headers
#)
