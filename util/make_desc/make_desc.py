#!/usr/bin/python
# ----------------------------------------------------------------------------
# Copyright (C) 2015 Verizon.  All Rights Reserved.
# All Rights Reserved
#
#   File:   make_desc.py
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
import pprint
import os
import sys
import re

# ------------------------------------------------------------------------------
# Globals
# ------------------------------------------------------------------------------
# Hidden policies
G_HIDE_POLICY_LIST = [
    'modsecurity_crs_23_request_limits.conf',
    'modsecurity_crs_30_http_policy.conf',
    'modsecurity_crs_49_inbound_blocking.conf',
    'modsecurity_crs_50_outbound.conf',
    'modsecurity_crs_59_outbound_blocking.conf'
]

G_DEFAULT_RULESET_DISPLAY_NAME = 'TODO_DESC_DISPLAY_NAME'

# ------------------------------------------------------------------------------
# 
# ------------------------------------------------------------------------------
def get_name_index_from_id(a_id):
    l_name = 'NA'
    l_index = 'NA'
    
    # Try OWASP_CRS
    m0 = re.match('^modsecurity_crs_(\d+)_(.*).conf', a_id)
    if m0:
        l_index = m0.group(1)
        l_name = m0.group(2)

    # Try GOT ROOT
    if not m0:
        m0 = re.match('^(\d+)_asl_(.*).conf', a_id)
        if m0:
            l_index = m0.group(1)
            l_name = m0.group(2)

    # Try MODSECURITY-COMMERCIAL-RULES
    if not m0:
        m0 = re.match('^modsecurity_slr_(\d+)_(.*).conf', a_id)
        if m0:
            l_index = m0.group(1)
            l_name = m0.group(2)

    # Replace '_' with ' ' in name
    l_name = l_name.replace('_', ' ')
    l_name = l_name.capitalize()
    
    return l_name, l_index

# ------------------------------------------------------------------------------
# 
# ------------------------------------------------------------------------------
def make_desc(a_display_name, a_unhide, a_rules_file, a_desc_json_file, a_verbose):
    
    if a_verbose:
        print 'Creating desc with rules file: %s'%(a_rules_file)

    # Read json
    with open(a_rules_file, 'r+') as l_rules_file:
        l_rules = json.load(l_rules_file)
        # TODO raise on error???

    l_desc = {}
    l_desc['display_name'] = a_display_name
    
    if a_unhide:
        l_desc['hidden'] = False
    else:
        l_desc['hidden'] = True

    # Create policy map
    l_policy_map = {}
    
    if 'sec_rule' not in l_rules:
        raise Exception('sec_rule property not found in input file: %s'%(a_rules_file))

    # Loop over rules
    # 1. Add display name "TODO_DESC_DISPLAY_NAME"
    # 2. Collapse actions into rules
    # 3. Hide rules...    
    for i_rule in l_rules['sec_rule']:
        
        # Get id from action
        if 'action' not in i_rule:
            raise Exception('rule missing action: %s'%(i_rule))

        l_new_rule = {}
        for i_key, i_val in i_rule.iteritems():
            if i_key == 'action':
                continue
            l_new_rule[i_key] = i_val
            
            for i_k, i_v in i_rule['action'].iteritems():
                l_new_rule[i_k] = i_v
                
        # Check for missing msg
        if 'msg' not in l_new_rule:
            l_name, l_index = get_name_index_from_id(l_new_rule['file'])
            l_new_rule['msg'] = '%s RULE_ID: %s'%(l_name, l_new_rule['id'])

        # Add rule to policy_map
        l_file = l_new_rule['file']
        if l_file not in l_policy_map:
            l_policy_map[l_file] = []
        l_policy_map[l_file].append(l_new_rule)
        

    # Dump policy map lists into description
    l_desc['policies'] = []
    for i_k, i_v in l_policy_map.iteritems():

        l_dict = {}
        
        # TODO extract name and index from id
        l_name, l_index = get_name_index_from_id(i_k)

        l_dict['name'] = l_name
        l_dict['index'] = l_index

        l_dict['id'] = i_k

        if i_k in G_HIDE_POLICY_LIST:
            l_dict['hidden'] = True
        else:
            l_dict['hidden'] = False

        l_dict['rules'] = i_v
        
        l_desc['policies'].append(l_dict)
    
    # Write desc into json
    with open(a_desc_json_file, 'w') as l_desc_file:
        l_desc_file.write(json.dumps(l_desc, indent=4, separators=(',', ': ')))

# ------------------------------------------------------------------------------
# main
# ------------------------------------------------------------------------------
def main(argv):
    
    arg_parser = argparse.ArgumentParser(
                description='Create desc file from rule.json.',
                usage= '%(prog)s',
                epilog= '')

    # Input file
    arg_parser.add_argument('-f',
                            '--file',
                            dest='rules_file',
                            help='File containing waf rules (json).',
                            required=True)

    # Output file
    arg_parser.add_argument('-o',
                            '--output',
                            dest='desc_json_file',
                            help='Description file output.',
                            required=True)

    # Ruleset display name
    arg_parser.add_argument('-n',
                            '--name',
                            dest='display_name',
                            help='Ruleset display name.',
                            default=G_DEFAULT_RULESET_DISPLAY_NAME,
                            required=False)
    
    # hide
    arg_parser.add_argument('-u',
                            '--unhide',
                            dest='unhide',
                            help='Unhide Ruleset.',
                            action='store_true',
                            default=False,
                            required=False)


    # verbose encoded file
    arg_parser.add_argument('-v',
                            '--verbose',
                            dest='verbose',
                            help='Verbosity.',
                            action='store_true',
                            default=False,
                            required=False)


    args = arg_parser.parse_args()

    make_desc(a_display_name = args.display_name,
              a_unhide = args.unhide,
              a_rules_file = args.rules_file,
              a_desc_json_file = args.desc_json_file,
              a_verbose = args.verbose)


# ------------------------------------------------------------------------------
#
# ------------------------------------------------------------------------------
if __name__ == "__main__":
    main(sys.argv[1:])

