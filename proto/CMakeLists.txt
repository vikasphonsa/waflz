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
project (waflz)

# ------------------------------------------------
# TODO Create PROTOBUF_GENERATE_PYTHON command
# for cmake...
# ------------------------------------------------
# Generate python bindings
# ------------------------------------------------
add_custom_command(
    OUTPUT waflz_pb2.py
    COMMAND protoc -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/waflz.proto --python_out=${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/waflz.proto
)

# custom target for protobuf output
add_custom_target(protodata_py_out ALL DEPENDS
    ${CMAKE_BINARY_DIR}/proto/waflz_pb2.py
)

# ------------------------------------------------
# ------------------------------------------------
add_custom_command(
    OUTPUT waflz.pb.cc waflz.pb.h
    COMMAND protoc -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/waflz.proto --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/waflz.proto
)


# ------------------------------------------------
# custom target for protobuf output
# ------------------------------------------------
add_custom_target(protobuf_cpp_out ALL DEPENDS
    ${CMAKE_BINARY_DIR}/proto/waflz.pb.cc
    ${CMAKE_BINARY_DIR}/proto/waflz.pb.h
)


# ------------------------------------------------
# Source files
# ------------------------------------------------

set(SRCS
   waflz.pb.cc
   sec_desc.pb.cc
)

# ------------------------------------------------
# Header files
# ------------------------------------------------
set(HDRS
    waflz.pb.h
)

# ------------------------------------------------
# Set INST_HDRS to ensure the build-time generated
# headers are included
# ------------------------------------------------
SET(INST_HDRS ${HDRS})
