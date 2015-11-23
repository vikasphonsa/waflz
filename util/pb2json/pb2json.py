#!/usr/bin/python
# ----------------------------------------------------------------------------
# Copyright (C) 2015 Verizon.  All Rights Reserved.
# All Rights Reserved
#
#   File:   pb2json.py
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

# ------------------------------------------------------------------------------
# Imports
# ------------------------------------------------------------------------------
import argparse
import json

import os
import sys

import pprint

# protobuf stuff
import google.protobuf
from copy import copy
from google.protobuf.descriptor import FieldDescriptor
from protobuf_to_dict import protobuf_to_dict, TYPE_CALLABLE_MAP
import waflz_pb2

# ------------------------------------------------------------------------------
# 
# ------------------------------------------------------------------------------
def pb2json_from_file(a_file):

    # --------------------------------------------
    # Read in pbuf file
    # --------------------------------------------
    with open(a_file) as l_data_file:
        l_pbuf_serialized = l_data_file.read()
    
    l_pbuf = waflz_pb2.config_t()
    try:
        l_pbuf.ParseFromString(l_pbuf_serialized)
    except Exception as e:
        g_logger.error('Error: performing pbuf parse. type: %s error: %s, doc: %s, message: %s'% (type(e), e, e.__doc__, e.message))
        raise
    
    # By default protobuf-to-dict converts byte arrays into base64 encoded strings,
    # but we want them to be actual byte arrays which we will parse and format
    # ourselves.
    l_type_callable_map = copy(TYPE_CALLABLE_MAP)
    l_type_callable_map[FieldDescriptor.TYPE_BYTES] = str

    # use_enum_labels causes protobuf_to_dict to use the enum names as strings rather
    # than the integer representation of that value
    l_dict = protobuf_to_dict(l_pbuf, type_callable_map=l_type_callable_map, use_enum_labels=True)
    # TODO try/except???
    
    print(json.dumps(l_dict))
    
# ------------------------------------------------------------------------------
# main
# ------------------------------------------------------------------------------
def main(argv):
    
    arg_parser = argparse.ArgumentParser(
                description='Generate Random Security Config Post from Template.',
                usage= '%(prog)s',
                epilog= '')

    # Template file
    arg_parser.add_argument('-f',
                            '--file',
                            dest='file',
                            help='waflz config (in pbuf).',
                            required=True)

    # verbose encoded file
    #arg_parser.add_argument('-v',
    #                        '--verbose',
    #                        dest='verbose',
    #                        help='Verbosity.',
    #                        action='store_true',
    #                        default=False,
    #                        required=False)

    args = arg_parser.parse_args()
    pb2json_from_file(args.file)


# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
if __name__ == "__main__":
    main(sys.argv[1:])
