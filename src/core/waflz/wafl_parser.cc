//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl_parser.cc
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    09/30/2015
//:
//:   Licensed under the Apache License, Version 2.0 (the "License");
//:   you may not use this file except in compliance with the License.
//:   You may obtain a copy of the License at
//:
//:       http://www.apache.org/licenses/LICENSE-2.0
//:
//:   Unless required by applicable law or agreed to in writing, software
//:   distributed under the License is distributed on an "AS IS" BASIS,
//:   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//:   See the License for the specific language governing permissions and
//:   limitations under the License.
//:
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "waflz.pb.h"
#include "ndebug.h"
#include "wafl_parser.h"
#include "util/util.h"
#include "jspb/jspb.h"
#include <google/protobuf/descriptor.h>

#include <errno.h>
#include <string.h>

// For whitespace...
#include <ctype.h>

// Directory traversal
#include <sys/types.h>
#include <dirent.h>

// File stat
#include <sys/stat.h>
#include <unistd.h>

#include <pcre.h>
#include <regex.h>

#include <set>
#include <algorithm>

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------

//: --------------------------------------------------------
//: Errors
//: --------------------------------------------------------
#define WAFLZ_CONFIG_ERROR_MSG(a_msg) \
        do { \
                show_config_error(__FILE__,__FUNCTION__,__LINE__,a_msg); \
        } while(0)

//: --------------------------------------------------------
//: Scanning
//: --------------------------------------------------------
#define SCAN_OVER_SPACE(l_line, l_char, l_line_len) \
        do { \
                while(isspace(int(*l_line)) && \
                      (l_char < l_line_len)) \
                {\
                        ++l_char;\
                        ++l_line;\
                }\
        } while(0)

#define SCAN_OVER_NON_SPACE_ESC(l_line, l_char, l_line_len) \
        do { \
                while((!isspace(int(*l_line)) || (isspace(int(*l_line)) && (*(l_line - 1) == '\\') && (*(l_line - 2) != '\\'))) && \
                     (l_char < l_line_len))\
                {\
                        ++l_char;\
                        ++l_line;\
                }\
        } while(0)

#define SCAN_OVER_SPACE_BKWD(l_line, l_char) \
        do { \
                while(isspace(int(*l_line)) && (l_char > 0))\
                {\
                        --l_char;\
                        --l_line;\
                }\
        } while(0)

#define SCAN_UNTIL_ESC(l_line, l_delimiter, l_char, l_line_len) \
        do { \
                while((((*l_line) != l_delimiter) || (((*l_line) == l_delimiter) && (*(l_line - 1) == '\\') && (*(l_line - 2) != '\\'))) && \
                     (l_char < l_line_len))\
                {\
                        ++l_char;\
                        ++l_line;\
                }\
        } while(0)

#define SCAN_UNTIL_ESC_QUOTE(l_line, l_delimiter, l_char, l_line_len) \
        do { \
                while((((*l_line) != l_delimiter) || (((*l_line) == l_delimiter) && (*(l_line - 1) == '\\') && (*(l_line - 2) != '\\'))) && \
                     (l_char < l_line_len))\
                {\
                        if((*l_line) == '\'') { ++l_char; ++l_line; SCAN_UNTIL_ESC(l_line, '\'', l_char, l_line_len); }\
                        if(l_char >= l_line_len) break; \
                        ++l_char;\
                        ++l_line;\
                }\
        } while(0)

//: --------------------------------------------------------
//: Caseless compare
//: --------------------------------------------------------
#define STRCASECMP_KV(a_match) (strcasecmp(i_kv->m_key.c_str(), a_match) == 0)
#define STRCASECMP(a_str, a_match) (strcasecmp(a_str.c_str(), a_match) == 0)

//: --------------------------------------------------------
//: Caseless compare helper for variables
//: --------------------------------------------------------
#define VARIABLE_SET_IF_KV(a_key) \
        if(STRCASECMP(l_variable_str, #a_key))\
        {\
                if(!i_kv->m_list.empty())\
                {\
                        l_var_type = waflz_pb::sec_rule_t_variable_t_type_t_##a_key;\
                }\
        }

//: --------------------------------------------------------
//: String 2 int
//: --------------------------------------------------------
#define STR2INT(a_str) strtoul(a_str.data(), NULL, 10)

//: ----------------------------------------------------------------------------
//: Fwd decl's
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::show_config_error(const char *a_file, const char *a_func, uint32_t a_line, const char *a_msg)
{

        NDBG_OUTPUT("%s.%s.%d: Error in file: %s line: %d:%d [%s]. Reason: %s\n",
                        a_file,
                        a_func,
                        a_line,
                        m_cur_file.c_str(),
                        m_cur_line_num,
                        m_cur_line_pos,
                        m_cur_line.c_str(),
                        a_msg
                        );

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::add_action(waflz_pb::sec_action_t *ao_action,
                                const kv_list_t & a_action_list,
                                bool &ao_is_chained)
{
        for(kv_list_t::const_iterator i_kv = a_action_list.begin();
            i_kv != a_action_list.end();
            ++i_kv)
        {

                // id
                if(STRCASECMP_KV("id"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_id(*(i_kv->m_list.begin()));
                        }
                }
                // msg
                else if(STRCASECMP_KV("msg"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_msg(*(i_kv->m_list.begin()));
                        }
                }
                // accuracy
                else if(STRCASECMP_KV("accuracy"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_accuracy(*(i_kv->m_list.begin()));
                        }
                }
                // capture
                else if(STRCASECMP_KV("capture"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_capture(true);
                        }
                }
                // ctl???
                else if(STRCASECMP_KV("ctl"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->add_ctl(*(i_kv->m_list.begin()));
                        }
                }
                // log
                else if(STRCASECMP_KV("log"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_log(true);
                        }
                }
                // logdata
                else if(STRCASECMP_KV("logdata"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_logdata(*(i_kv->m_list.begin()));
                        }
                }
                // maturity
                else if(STRCASECMP_KV("maturity"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_maturity(*(i_kv->m_list.begin()));
                        }
                }
                // multimatch
                else if(STRCASECMP_KV("multimatch"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_multimatch(true);
                        }
                }
                // noauditlog
                else if(STRCASECMP_KV("noauditlog"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_noauditlog(true);
                        }
                }
                // auditlog
                else if(STRCASECMP_KV("auditlog"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_auditlog(true);
                        }
                }
                // sanitisematched
                else if(STRCASECMP_KV("sanitisematched"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_sanitisematched(true);
                        }
                }
                // initcol
                else if(STRCASECMP_KV("initcol"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_initcol(*(i_kv->m_list.begin()));
                        }
                }
                // status
                else if(STRCASECMP_KV("status"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_status(*(i_kv->m_list.begin()));
                        }
                }
                // initcol
                else if(STRCASECMP_KV("skip"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_skip(STR2INT((*(i_kv->m_list.begin()))));
                        }
                }
                // noauditlog
                else if(STRCASECMP_KV("nolog"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_nolog(true);
                        }
                }
                // phase
                else if(STRCASECMP_KV("phase"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_phase(STR2INT((*(i_kv->m_list.begin()))));
                        }
                }
                // rev
                else if(STRCASECMP_KV("rev"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_rev(*(i_kv->m_list.begin()));
                        }
                }
                // severity
                else if(STRCASECMP_KV("severity"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_severity(STR2INT((*(i_kv->m_list.begin()))));
                        }
                }
                // setvar
                else if(STRCASECMP_KV("setvar"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                for(string_list_t::const_iterator i_t = i_kv->m_list.begin();
                                    i_t != i_kv->m_list.end();
                                    ++i_t)
                                {
                                        ao_action->add_setvar(*i_t);
                                }
                        }
                }
                // skipAfter
                else if(STRCASECMP_KV("skipafter"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_skipafter(*(i_kv->m_list.begin()));
                        }
                }
                // tag
                else if(STRCASECMP_KV("tag"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                for(string_list_t::const_iterator i_t = i_kv->m_list.begin();
                                    i_t != i_kv->m_list.end();
                                    ++i_t)
                                {
                                        ao_action->add_tag(*i_t);
                                }
                        }
                }
                // ver
                else if(STRCASECMP_KV("ver"))
                {
                        // Use first
                        if(!i_kv->m_list.empty())
                        {
                                ao_action->set_ver(*(i_kv->m_list.begin()));
                        }
                }
                // actions
                else if(STRCASECMP_KV("pass"))
                {
                        ao_action->set_action_type(waflz_pb::sec_action_t_action_type_t_PASS);

                }
                else if(STRCASECMP_KV("block"))
                {
                        ao_action->set_action_type(waflz_pb::sec_action_t_action_type_t_BLOCK);

                }
                else if(STRCASECMP_KV("deny"))
                {
                        ao_action->set_action_type(waflz_pb::sec_action_t_action_type_t_DENY);

                }
                else if(STRCASECMP_KV("t"))
                {

                        // Add transforms for list
                        for(string_list_t::const_iterator i_t = i_kv->m_list.begin();
                            i_t != i_kv->m_list.end();
                            ++i_t)
                        {
                                if(STRCASECMP((*i_t), "CMDLINE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_CMDLINE);
                                }
                                else if(STRCASECMP((*i_t), "COMPRESSWHITESPACE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_COMPRESSWHITESPACE);
                                }
                                else if(STRCASECMP((*i_t), "CSSDECODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_CSSDECODE);
                                }
                                else if(STRCASECMP((*i_t), "HEXENCODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_HEXENCODE);
                                }
                                else if(STRCASECMP((*i_t), "HEXDECODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_HEXDECODE);
                                }
                                else if(STRCASECMP((*i_t), "HTMLENTITYDECODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_HTMLENTITYDECODE);
                                }
                                else if(STRCASECMP((*i_t), "JSDECODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_JSDECODE);
                                }
                                else if(STRCASECMP((*i_t), "LENGTH"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_LENGTH);
                                }
                                else if(STRCASECMP((*i_t), "LOWERCASE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_LOWERCASE);
                                }
                                else if(STRCASECMP((*i_t), "MD5"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_MD5);
                                }
                                else if(STRCASECMP((*i_t), "NONE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_NONE);
                                }
                                else if(STRCASECMP((*i_t), "NORMALISEPATH"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_NORMALIZEPATH);
                                }
                                else if(STRCASECMP((*i_t), "NORMALIZEPATHWIN"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_NORMALIZEPATHWIN);
                                }
                                else if(STRCASECMP((*i_t), "REMOVENULLS"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_REMOVENULLS);
                                }
                                else if(STRCASECMP((*i_t), "REMOVEWHITESPACE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_REMOVEWHITESPACE);
                                }
                                else if(STRCASECMP((*i_t), "REPLACECOMMENTS"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_REPLACECOMMENTS);
                                }
                                else if(STRCASECMP((*i_t), "SHA1"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_SHA1);
                                }
                                else if(STRCASECMP((*i_t), "URLDECODEUNI"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_URLDECODEUNI);
                                }
                                else if(STRCASECMP((*i_t), "UTF8TOUNICODE"))
                                {
                                        ao_action->add_t(waflz_pb::sec_action_t_transformation_type_t_UTF8TOUNICODE);
                                }
                                else
                                {
                                        std::string l_lowercase = *i_t;
                                        std::transform(l_lowercase.begin(), l_lowercase.end(), l_lowercase.begin(), ::tolower);
                                        ++(m_unimplemented_transformations[l_lowercase]);
                                }
                        }

                }
                // chain
                else if(STRCASECMP_KV("chain"))
                {
                        ao_is_chained = true;
                }
                // default
                else
                {
                        std::string l_lowercase = i_kv->m_key;
                        std::transform(l_lowercase.begin(), l_lowercase.end(), l_lowercase.begin(), ::tolower);
                        ++(m_unimplemented_actions[l_lowercase]);
                }
        }

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::add_rule(const kv_list_t &a_variable_list,
                               const std::string &a_operator_fx,
                               const std::string &a_operator_match,
                               const kv_list_t &a_action_list)
{

        waflz_pb::sec_rule_t *l_rule = NULL;
        bool l_is_chained = false;

        // Add rule to config or to chain
        if(m_cur_parent_rule) {
                l_rule = m_cur_parent_rule->add_chained_rule();
        } else {
                l_rule = m_config->add_sec_rule();
                l_rule->set_order(m_config->sec_rule_size() - 1);
        }

        // -----------------------------------------------------------
        // variables...
        // -----------------------------------------------------------
        for(kv_list_t::const_iterator i_kv = a_variable_list.begin();
            i_kv != a_variable_list.end();
            ++i_kv)
        {
                bool l_found_key = true;

                // Loop over list
                for(string_list_t::const_iterator i_v = i_kv->m_list.begin();
                    i_v != i_kv->m_list.end();
                    ++i_v)
                {
                        std::string l_variable_str;

                        // Get first character to determine if count or negation
                        // Negation
                        bool l_match_is_negated = false;
                        bool l_is_count = false;

                        if(i_kv->m_key.at(0) == '!')
                        {
                                l_match_is_negated = true;
                                l_variable_str.assign(i_kv->m_key.data() + 1, i_kv->m_key.length() - 1);
                        }
                        else if(i_kv->m_key.at(0) == '&')
                        {
                                l_is_count = true;
                                l_variable_str.assign(i_kv->m_key.data() + 1, i_kv->m_key.length() - 1);
                        }
                        else
                        {
                                l_variable_str = i_kv->m_key;
                        }

                        // ---------------------------------------------------
                        // get type
                        // ---------------------------------------------------
                        waflz_pb::sec_rule_t_variable_t_type_t l_var_type = waflz_pb::sec_rule_t_variable_t_type_t_ARGS;
                        VARIABLE_SET_IF_KV(ARGS)
                        else VARIABLE_SET_IF_KV(ARGS_COMBINED_SIZE)
                        else VARIABLE_SET_IF_KV(ARGS_NAMES)
                        else VARIABLE_SET_IF_KV(FILES)
                        else VARIABLE_SET_IF_KV(FILES_COMBINED_SIZE)
                        else VARIABLE_SET_IF_KV(FILES_NAMES)
                        else VARIABLE_SET_IF_KV(GLOBAL)
                        else VARIABLE_SET_IF_KV(MULTIPART_STRICT_ERROR)
                        else VARIABLE_SET_IF_KV(MULTIPART_UNMATCHED_BOUNDARY)
                        else VARIABLE_SET_IF_KV(QUERY_STRING)
                        else VARIABLE_SET_IF_KV(REMOTE_ADDR)
                        else VARIABLE_SET_IF_KV(REQBODY_ERROR)
                        else VARIABLE_SET_IF_KV(REQUEST_BASENAME)
                        else VARIABLE_SET_IF_KV(REQUEST_BODY)
                        else VARIABLE_SET_IF_KV(REQUEST_COOKIES)
                        else VARIABLE_SET_IF_KV(REQUEST_COOKIES_NAMES)
                        else VARIABLE_SET_IF_KV(REQUEST_FILENAME)
                        else VARIABLE_SET_IF_KV(REQUEST_HEADERS)
                        else VARIABLE_SET_IF_KV(REQUEST_HEADERS_NAMES)
                        else VARIABLE_SET_IF_KV(REQUEST_LINE)
                        else VARIABLE_SET_IF_KV(REQUEST_METHOD)
                        else VARIABLE_SET_IF_KV(REQUEST_PROTOCOL)
                        else VARIABLE_SET_IF_KV(REQUEST_URI)
                        else VARIABLE_SET_IF_KV(RESOURCE)
                        else VARIABLE_SET_IF_KV(RESPONSE_BODY)
                        else VARIABLE_SET_IF_KV(RESPONSE_STATUS)
                        else VARIABLE_SET_IF_KV(TX)
                        else VARIABLE_SET_IF_KV(WEBSERVER_ERROR_LOG)
                        else VARIABLE_SET_IF_KV(XML)
                        else VARIABLE_SET_IF_KV(REQBODY_PROCESSOR)
                        else VARIABLE_SET_IF_KV(ARGS_GET)
                        else VARIABLE_SET_IF_KV(ARGS_GET_NAMES)
                        else VARIABLE_SET_IF_KV(ARGS_POST)
                        else VARIABLE_SET_IF_KV(ARGS_POST_NAMES)
                        else VARIABLE_SET_IF_KV(MATCHED_VAR)
                        else VARIABLE_SET_IF_KV(RESPONSE_HEADERS)
                        else VARIABLE_SET_IF_KV(SESSION)
                        // default
                        else
                        {
                                std::string l_lowercase = l_variable_str;
                                std::transform(l_lowercase.begin(), l_lowercase.end(), l_lowercase.begin(), ::tolower);
                                ++(m_unimplemented_variables[l_lowercase]);
                                l_found_key = false;
                        }

                        waflz_pb::sec_rule_t::variable_t::match_t *l_match = NULL;
                        if(l_found_key && !(i_v->empty()))
                        {
                                l_match = new waflz_pb::sec_rule_t::variable_t::match_t();
                                l_match->set_is_negated(l_match_is_negated);

                                std::string l_match_str = *i_v;

                                // Check if has "selection operator" '/'s
                                if((l_match_str[0] == '/') && (l_match_str[l_match_str.length() - 1] == '/'))
                                {
                                        l_match->set_value(l_match_str.substr(1, l_match_str.length() - 2));
                                        l_match->set_is_regex(true);
                                }
                                else
                                {
                                        l_match->set_value(l_match_str);
                                        l_match->set_is_regex(false);
                                }
                        }
                        else if(l_found_key)
                        {
                                l_match = new waflz_pb::sec_rule_t::variable_t::match_t();
                                l_match->set_is_negated(false);
                                l_match->set_is_regex(false);
                        }

                        // Find existing type?
                        int32_t i_var = 0;
                        bool l_found_var = false;
                        for(i_var = 0; i_var < l_rule->variable_size(); ++i_var)
                        {
                                if(l_rule->variable(i_var).type() == l_var_type)
                                {
                                        l_found_var = true;
                                        break;
                                }
                        }

                        if(!l_found_var)
                        {
                                waflz_pb::sec_rule_t::variable_t *l_variable = l_rule->add_variable();
                                l_variable->set_type(l_var_type);
                                l_variable->set_is_count(l_is_count);

                                if(l_match)
                                {
                                        l_variable->add_match()->CopyFrom(*l_match);
                                }
                        }
                        else
                        {
                                // Add match to existing var
                                if(l_match)
                                {
                                        l_rule->mutable_variable(i_var)->add_match()->CopyFrom(*l_match);
                                }
                        }

                        if(l_match)
                        {
                                delete l_match;
                        }

                }

        }

        // -----------------------------------------------------------
        // operator...
        // -----------------------------------------------------------
        if(!a_operator_fx.empty() || !a_operator_match.empty())
        {
                waflz_pb::sec_rule_t::operator_t *l_operator = l_rule->mutable_operator_();
                if(!a_operator_fx.empty())
                {

                        l_operator->set_is_regex(false);

                        // -------------------------------------------
                        //
                        // -------------------------------------------
                        //
                        if(STRCASECMP(a_operator_fx, "BEGINSWITH"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_BEGINSWITH);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "CONTAINS"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_CONTAINS);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "CONTAINSWORD"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_CONTAINSWORD);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "ENDSWITH"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_ENDSWITH);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "EQ"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_EQ);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "GE"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_GE);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "GT"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_GT);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "LT"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_LT);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "PM"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_PM);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "PMFROMFILE"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_PMFROMFILE);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "PMF"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_PMF);
                        }
                        // rx
                        else if(STRCASECMP(a_operator_fx, "RX"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_RX);
                                l_operator->set_is_regex(true);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "STREQ"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_STREQ);
                        }

                        //
                        else if(STRCASECMP(a_operator_fx, "STRMATCH"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_STRMATCH);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "VALIDATEBYTERANGE"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_VALIDATEBYTERANGE);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "VALIDATEURLENCODING"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_VALIDATEURLENCODING);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "VALIDATEUTF8ENCODING"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_VALIDATEUTF8ENCODING);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "VERIFYCC"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_VERIFYCC);
                        }
                        //
                        else if(STRCASECMP(a_operator_fx, "WITHIN"))
                        {
                                l_operator->set_type(waflz_pb::sec_rule_t_operator_t_type_t_WITHIN);
                        }
                        // default
                        else
                        {
                                std::string l_lowercase = a_operator_fx;
                                std::transform(l_lowercase.begin(), l_lowercase.end(), l_lowercase.begin(), ::tolower);
                                ++(m_unimplemented_operators[l_lowercase]);
                        }
                }
                else if(!a_operator_match.empty())
                {
                        l_operator->set_is_regex(true);
                }

                if(!a_operator_match.empty())
                {
                        l_operator->set_value(a_operator_match);
                }
        }


        // -----------------------------------------------------------
        // actions...
        // -----------------------------------------------------------
        // Add action
        waflz_pb::sec_action_t *l_action = l_rule->mutable_action();
        int32_t l_status;
        l_status = add_action(l_action, a_action_list, l_is_chained);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        // -----------------------------------------------------------
        // handle chain
        // -----------------------------------------------------------
        if(!l_is_chained)
        {
                m_cur_parent_rule = NULL;
        }
        else if(!m_cur_parent_rule)
        {
                m_cur_parent_rule = l_rule;
        }

        // Set file
        l_action->set_file(m_cur_file_base);

        // Set hidden to false
        l_rule->set_hidden(false);

        return STATUS_OK;

}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::tokenize_kv_list(const std::string &a_string,
                                       const char a_delimiter,
                                       kv_list_t &ao_kv_list)
{
        // TODO Add arg to specify kv delimiter -hard coded to ':' now

        // Parse list by characters
        // Scan over string copying bits into list until end-of-string
        uint32_t i_char = 0;
        const char *l_line = a_string.data();
        uint32_t l_line_len = a_string.length();

        while(i_char < l_line_len)
        {
                const char *l_str_begin = l_line;
                uint32_t l_str_begin_index = i_char;
                SCAN_UNTIL_ESC_QUOTE(l_line, a_delimiter, i_char, l_line_len);
                std::string l_part;
                l_part.assign(l_str_begin, i_char - l_str_begin_index);

                //NDBG_PRINT("PART[%d]: %s\n", 0, l_part.c_str());

                // Now we have a string -that is optional split by colon's
                std::string l_key;
                std::string l_val = "";
                const char *l_str_key = l_part.data();
                uint32_t i_char_key = 0;

                SCAN_UNTIL_ESC_QUOTE(l_str_key, ':', i_char_key, l_part.length());
                l_key.assign(l_part.data(), i_char_key);
                if(i_char_key < l_part.length())
                {
                        l_val.assign(l_str_key + 1, l_part.length() - i_char_key);
                }

                // Clean quotes
                if (!l_val.empty() && l_val.at(0) == '\'' )
                {
                        std::string l_val_tmp = l_val;
                        l_val.assign(l_val_tmp.data() + 1, l_val_tmp.length() - 3);
                }

                std::string l_val_tmp = l_val.c_str();
                l_val = l_val_tmp;

                //NDBG_PRINT("KEY[%s]: %s\n", l_key.c_str(), l_val.c_str());
                //ao_string_list.push_back(l_part);

                // find in list
                kv_list_t::iterator i_kv;
                for(i_kv = ao_kv_list.begin(); i_kv != ao_kv_list.end(); ++i_kv)
                {
                        if(i_kv->m_key == l_key)
                                break;
                }


                if(i_kv == ao_kv_list.end())
                {
                        kv_t l_kv;
                        l_kv.m_key = l_key;
                        l_kv.m_list.push_back(l_val);

                        ao_kv_list.push_back(l_kv);
                }
                else
                {
                        i_kv->m_list.push_back(l_val);
                }

                if(i_char < l_line_len)
                {
                        ++i_char;
                        ++l_line;

                        // chomp whitespace
                        SCAN_OVER_SPACE(l_line, i_char, l_line_len);
                        if(i_char >= l_line_len)
                        {
                                break;
                        }
                }
                else
                {
                        break;
                }
        }

        if(m_verbose)
        {
                for(kv_list_t::iterator i_kv = ao_kv_list.begin();
                    i_kv != ao_kv_list.end();
                    ++i_kv)
                {
                        uint32_t i_var = 0;
                        for(string_list_t::iterator i_v = i_kv->m_list.begin();
                            i_v != i_kv->m_list.end();
                            ++i_v, ++i_var)
                        {
                                NDBG_OUTPUT("KEY: %24s[%3d]: %s\n", i_kv->m_key.c_str(), i_var, i_v->c_str());
                        }
                }
        }

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::get_next_string(char **ao_line,
                                     uint32_t *ao_char,
                                     uint32_t a_line_len,
                                     std::string &ao_string)
{

        //NDBG_PRINT("%sao_line%s: %.*s\n",  ANSI_COLOR_FG_RED, ANSI_COLOR_OFF, a_line_len, (*ao_line));

        // Scan past whitespace to first quote
        SCAN_OVER_SPACE(*ao_line, *ao_char, a_line_len);
        if(*ao_char == a_line_len)
        {
                return STATUS_OK;
        }

        bool l_is_quoted = true;
        if((*(*ao_line)) != '\"')
        {
                if(isgraph(int((*(*ao_line)))) || (int((*(*ao_line))) == '&'))
                {
                        l_is_quoted = false;
                        //NDBG_PRINT("%sNO_QUOTE%s: l_line: %s\n", ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, (*ao_line));
                }
                else
                {
                        WAFLZ_CONFIG_ERROR_MSG("isgraph");
                        return STATUS_ERROR;
                }
        }

        if(l_is_quoted)
        {
                ++(*ao_char);
                ++(*ao_line);
        }

        const char *l_str_begin = *ao_line;
        uint32_t l_str_begin_index = *ao_char;
        //NDBG_PRINT("START[%d]: %s\n",  l_str_begin_index, *ao_line);

        if(l_is_quoted)
        {
                SCAN_UNTIL_ESC(*ao_line, '"', *ao_char, a_line_len);
                if((*ao_char) == a_line_len)
                {
                        ao_string.assign(l_str_begin, *ao_char - l_str_begin_index - 1);
                        return STATUS_OK;
                }
        }
        else
        {
                SCAN_OVER_NON_SPACE_ESC(*ao_line, *ao_char, a_line_len);
        }

        //NDBG_PRINT("STR: %.*s\n",  *ao_char - l_str_begin_index, l_str_begin);
        //NDBG_PRINT("END: %d\n", *ao_char - l_str_begin_index);

        ao_string.assign(l_str_begin, *ao_char - l_str_begin_index);

        if(l_is_quoted)
        {
                ++(*ao_char);
                ++(*ao_line);
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::get_strings_from_line(const char *a_line,
                                           uint32_t a_line_len,
                                           string_list_t &ao_str_list)
{
        const char *l_line = a_line;
        uint32_t l_char = 0;
        uint32_t l_line_len = a_line_len;
        int32_t l_status = STATUS_OK;

        std::string l_str;
        do {
                l_str.clear();
                l_status = get_next_string((char **)&l_line, &l_char, l_line_len, l_str);
                if((l_status == STATUS_OK) && !l_str.empty())
                {
                        //NDBG_PRINT("l_status: %d l_str: %s\n", l_status, l_str.c_str());
                        ao_str_list.push_back(l_str);
                }
        } while((l_status == STATUS_OK) && !l_str.empty());

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \notes:
//:   Syntax:        SecRule  VARIABLES  OPERATOR      [ACTIONS]
//:   Example Usage: SecRule  ARGS       "@rx attack"  "phase:1,log,deny,id:1"
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_secaction(const char *a_line,
                                    uint32_t a_line_len,
                                    bool a_is_default)
{
        uint32_t i_char = 0;
        const char *l_line = a_line;
        uint32_t l_line_len = a_line_len;
        int32_t l_status;

        //NDBG_OUTPUT("------------------------------------------------------------------\n");
        //NDBG_OUTPUT("FILE: %s %sLINE%s[%d] %sDIRECTIVE%s: %s: %s\n",
        //                m_cur_file.c_str(),
        //                ANSI_COLOR_FG_YELLOW, ANSI_COLOR_OFF, m_cur_line_num,
        //                ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, "SEC_RULE",
        //                a_line);

        // Scan past whitespace
        SCAN_OVER_SPACE(l_line, i_char, l_line_len);
        if(i_char == l_line_len)
        {
                return STATUS_OK;
        }

        // ---------------------------------------
        // Get ACTIONS [optional]
        // ---------------------------------------
        std::string l_actions;
        l_status = get_next_string((char **)&l_line, &i_char, l_line_len, l_actions);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        //NDBG_PRINT("l_actions: %s\n", l_actions.c_str());

        if(m_verbose)
        {
                if(m_color)
                {
                        NDBG_OUTPUT("%sLINE%s[%d] %s%s%s\n",
                                        ANSI_COLOR_FG_WHITE, ANSI_COLOR_OFF, m_cur_line_num,
                                        ANSI_COLOR_FG_YELLOW, l_actions.c_str(), ANSI_COLOR_OFF);
                }
                else
                {
                        NDBG_OUTPUT("LINE[%d] %s\n",
                                        m_cur_line_num,
                                        l_actions.c_str());
                }
        }

        // ---------------------------------------
        // Try parse actions
        // ---------------------------------------
        kv_list_t l_action_list;
        l_status = tokenize_kv_list(l_actions, ',', l_action_list);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Add rule
        // ---------------------------------------
        waflz_pb::sec_action_t *l_action = NULL;
        if(a_is_default)
        {
                l_action = m_config->mutable_default_action();
        }
        else
        {
                l_action = m_config->add_action();
        }
        bool l_unused;
        l_status = add_action(l_action, l_action_list, l_unused);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \notes:
//:   Syntax:        SecRule  VARIABLES  OPERATOR      [ACTIONS]
//:   Example Usage: SecRule  ARGS       "@rx attack"  "phase:1,log,deny,id:1"
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_secrule(const char *a_line,
                                   uint32_t a_line_len)
{
        uint32_t i_char = 0;
        const char *l_line = a_line;
        uint32_t l_line_len = a_line_len;
        int32_t l_status;

        //NDBG_OUTPUT("------------------------------------------------------------------\n");
        //NDBG_OUTPUT("FILE: %s %sLINE%s[%d] %sDIRECTIVE%s: %s: %s\n",
        //                m_cur_file.c_str(),
        //                ANSI_COLOR_FG_YELLOW, ANSI_COLOR_OFF, m_cur_line_num,
        //                ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, "SEC_RULE",
        //                a_line);

        // Scan past whitespace
        SCAN_OVER_SPACE(l_line, i_char, l_line_len);
        if(i_char == l_line_len)
        {
                return STATUS_OK;
        }

        // ---------------------------------------
        // Get VARIABLES
        // ---------------------------------------
        std::string l_variables;
        l_status = get_next_string((char **)&l_line, &i_char, l_line_len, l_variables);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        //NDBG_PRINT("l_variables: %s\n", l_variables.c_str());

        // ---------------------------------------
        // Get OPERATOR
        // ---------------------------------------
        std::string l_operator;
        l_status = get_next_string((char **)&l_line, &i_char, l_line_len, l_operator);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        //NDBG_PRINT("l_operator: %s\n", l_operator.c_str());

        // ---------------------------------------
        // Get ACTIONS [optional]
        // ---------------------------------------
        std::string l_actions;
        l_status = get_next_string((char **)&l_line, &i_char, l_line_len, l_actions);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        //NDBG_PRINT("l_actions: %s\n", l_actions.c_str());

        if(m_verbose)
        {
                if(m_color)
                {
                        NDBG_OUTPUT("%sLINE%s[%d] %s%s%s %s%s%s %s%s%s\n",
                                        ANSI_COLOR_FG_WHITE, ANSI_COLOR_OFF, m_cur_line_num,
                                        ANSI_COLOR_FG_GREEN,  l_variables.c_str(), ANSI_COLOR_OFF,
                                        ANSI_COLOR_FG_BLUE,   l_operator.c_str(), ANSI_COLOR_OFF,
                                        ANSI_COLOR_FG_YELLOW, l_actions.c_str(), ANSI_COLOR_OFF);
                }
                else
                {
                        NDBG_OUTPUT("LINE[%d] %s %s %s\n",
                                        m_cur_line_num,
                                        l_variables.c_str(),
                                        l_operator.c_str(),
                                        l_actions.c_str());
                }
        }

        // ---------------------------------------
        // Try parse variables
        // ---------------------------------------
        kv_list_t l_variable_list;
        l_status = tokenize_kv_list(l_variables, '|', l_variable_list);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Parse operator
        // ---------------------------------------
        std::string l_operator_fx;
        std::string l_operator_match;

        // Special checks for @ prefix
        // const std::string &a_operator,
        //NDBG_PRINT("%sOPERATOR%s: %s\n", ANSI_COLOR_FG_BLUE, ANSI_COLOR_OFF, l_operator.c_str());
        if(l_operator.at(0) == '@')
        {
                // Scan until space
                const char *l_fx_line = l_operator.data();
                uint32_t l_fx_char = 0;
                uint32_t l_fx_line_len = l_operator.length();
                std::string l_fx;
                SCAN_OVER_NON_SPACE_ESC(l_fx_line, l_fx_char, l_fx_line_len);
                l_operator_fx.assign(l_operator.data() + 1, l_fx_char - 1);
                if(l_fx_char < l_fx_line_len)
                {
                        SCAN_OVER_SPACE(l_fx_line, l_fx_char, l_fx_line_len);
                        l_operator_match.assign(l_fx_line, l_fx_line_len - l_fx_char);
                }
                //NDBG_PRINT("OPERATOR[%s]: %s\n", l_operator_fx.c_str(), l_operator_match.c_str());

        }
        else
        {
                l_operator_fx = "";
                l_operator_match = l_operator;
        }



        // ---------------------------------------
        // Try parse actions
        // ---------------------------------------
        kv_list_t l_action_list;
        l_status = tokenize_kv_list(l_actions, ',', l_action_list);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Add rule
        // ---------------------------------------
        l_status = add_rule(l_variable_list, l_operator_fx, l_operator_match, l_action_list);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_wholeline(const char *a_line,
                                     uint32_t a_line_len)
{
        uint32_t i_char = 0;
        const char *l_line = a_line;
        uint32_t l_line_len = a_line_len;

        // -------------------------------------------------
        // Get directive string
        // Space delimited -read until space
        // -------------------------------------------------
        const char *l_directive_str_begin = l_line;
        SCAN_OVER_NON_SPACE_ESC(l_line, i_char, l_line_len);
        if(i_char == l_line_len)
        {
                return STATUS_OK;
        }

        std::string l_directive;
        l_directive.assign(l_directive_str_begin, l_line);

        // Scan past whitespace
        SCAN_OVER_SPACE(l_line, i_char, l_line_len);
        if(i_char == l_line_len)
        {
                return STATUS_OK;
        }

        //NDBG_PRINT("DIRECTIVE: %s\n", l_directive.c_str());

        // -------------------------------------------------
        // Include Directive --recurse!!!
        // -------------------------------------------------
        if(strcasecmp(l_directive.c_str(), "include") == 0)
        {

                //NDBG_OUTPUT(": %s\n", l_line);

                // Read from quote to quote
                if(*l_line != '"')
                {
                        // TODO Make error macro to print file/line/cursor
                        WAFLZ_CONFIG_ERROR_MSG("include file specification malformed");
                        return STATUS_ERROR;
                }

                // Skip quote
                ++i_char;
                ++l_line;

                const char *l_include_file_str_start = l_line;
                SCAN_UNTIL_ESC(l_line, '"', i_char, l_line_len);
                // Check bounds
                if(i_char == l_line_len)
                {
                        // TODO Make error macro to print file/line/cursor
                        WAFLZ_CONFIG_ERROR_MSG("include file specification malformed");
                        return STATUS_ERROR;
                }

                std::string l_include_file;
                l_include_file.assign(l_include_file_str_start, l_line);
                //NDBG_OUTPUT("%sINCLUDE%s: %s\n", ANSI_COLOR_FG_MAGENTA, ANSI_COLOR_OFF, l_include_file.c_str());

                // Recurse
                int l_status;
                l_status = read_file_modsec(l_include_file.c_str(), false);
                if(l_status != STATUS_OK)
                {
                        return STATUS_ERROR;
                }

        } else {

                string_list_t l_list;

                // -----------------------------------------
                // secrule
                // -----------------------------------------
#define IF_DIRECTIVE(_a_dir) if(strcasecmp(l_directive.c_str(), _a_dir) == 0)
#define ELIF_DIRECTIVE(_a_dir) else if(strcasecmp(l_directive.c_str(), _a_dir) == 0)

                IF_DIRECTIVE("secrule")
                {
                        int l_status;
                        l_status = read_secrule(l_line, a_line_len - i_char);
                        if(l_status != STATUS_OK)
                        {
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secdefaultaction")
                {
                        int l_status;
                        l_status = read_secaction(l_line, a_line_len - i_char, true);
                        if(l_status != STATUS_OK)
                        {
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secaction")
                {
                        int l_status;
                        l_status = read_secaction(l_line, a_line_len - i_char, false);
                        if(l_status != STATUS_OK)
                        {
                                return STATUS_ERROR;
                        }
                }

#define GET_STRS(_error_msg) \
        do {\
        int32_t _status = 0;\
        _status = get_strings_from_line(l_line, a_line_len - i_char, l_list);\
        if((_status != STATUS_OK) || l_list.empty())\
        {\
                WAFLZ_CONFIG_ERROR_MSG(_error_msg);\
                return STATUS_ERROR;\
        }\
        } while(0)

                ELIF_DIRECTIVE("secargumentseparator")
                {
                        GET_STRS("secargumentseparator missing separator");
                        m_config->set_argument_separator((uint32_t)(l_list.begin()->data()[0]));
                }
                ELIF_DIRECTIVE("seccomponentsignature")
                {
                        GET_STRS("seccomponentsignature missing string");
                        m_config->set_component_signature(*(l_list.begin()));
                }
                ELIF_DIRECTIVE("seccookieformat")
                {
                        GET_STRS("seccookieformat missing format type 0|1");
                        if(*(l_list.begin()) == "1")
                        {
                                m_config->set_cookie_format(1);
                        }
                        else
                        {
                                m_config->set_cookie_format(0);
                        }
                }
                ELIF_DIRECTIVE("secdatadir")
                {
                        GET_STRS("secdatadir missing directory string");
                        m_config->set_data_dir(*(l_list.begin()));
                }
                ELIF_DIRECTIVE("secmarker")
                {
                        GET_STRS("secmarker missing id|label string");
                        waflz_pb::sec_config_t_marker_t *l_marker = m_config->add_marker();
                        l_marker->set_label(*(l_list.begin()));

                        // Set mark to last rule order
                        if(m_config->sec_rule_size())
                        {
                                l_marker->set_mark(m_config->sec_rule(m_config->sec_rule_size() - 1).order());
                        }
                        else
                        {
                                l_marker->set_mark(0);
                        }

                }
                ELIF_DIRECTIVE("secpcrematchlimit")
                {
                        GET_STRS("secpcrematchlimit missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_pcre_match_limit(l_limit);
                }
                ELIF_DIRECTIVE("secpcrematchlimitrecursion")
                {
                        GET_STRS("secpcrematchlimitrecursion missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_pcre_match_limit_recursion(l_limit);
                }
                ELIF_DIRECTIVE("secrequestbodyaccess")
                {
                        GET_STRS("secrequestbodyaccess missing specifier on|off");
                        if STRCASECMP((*(l_list.begin())), "ON")
                        {
                                m_config->set_request_body_access(true);
                        }
                        else if STRCASECMP((*(l_list.begin())), "OFF")
                        {
                                m_config->set_request_body_access(false);
                        }
                        else
                        {
                                WAFLZ_CONFIG_ERROR_MSG("secrequestbodyaccess missing specifier on|off");
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secrequestbodyinmemorylimit")
                {
                        GET_STRS("secrequestbodyinmemorylimit missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_request_body_in_memory_limit(l_limit);
                }
                ELIF_DIRECTIVE("secrequestbodylimit")
                {
                        GET_STRS("secrequestbodylimit missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_request_body_limit(l_limit);
                }
                ELIF_DIRECTIVE("secrequestbodylimitaction")
                {
                        GET_STRS("secrequestbodylimitaction missing specifier Reject|ProcessPartial");
                        if STRCASECMP((*(l_list.begin())), "Reject")
                        {
                                m_config->set_request_body_limit_action(waflz_pb::sec_config_t_limit_action_type_t_REJECT);
                        }
                        else if STRCASECMP((*(l_list.begin())), "ProcessPartial")
                        {
                                m_config->set_request_body_limit_action(waflz_pb::sec_config_t_limit_action_type_t_PROCESS_PARTIAL);
                        }
                        else
                        {
                                WAFLZ_CONFIG_ERROR_MSG("secrequestbodylimitaction missing specifier Reject|ProcessPartial");
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secrequestbodynofileslimit")
                {
                        GET_STRS("secrequestbodynofileslimit missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_request_body_no_files_limit(l_limit);
                }
                ELIF_DIRECTIVE("secresponsebodyaccess")
                {
                        GET_STRS("secresponsebodyaccess missing specifier on|off");
                        if STRCASECMP((*(l_list.begin())), "ON")
                        {
                                m_config->set_response_body_access(true);
                        }
                        else if STRCASECMP((*(l_list.begin())), "OFF")
                        {
                                m_config->set_response_body_access(false);
                        }
                        else
                        {
                                WAFLZ_CONFIG_ERROR_MSG("secresponsebodyaccess missing specifier on|off");
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secresponsebodylimit")
                {
                        GET_STRS("secresponsebodylimit missing size");
                        uint32_t l_limit = STR2INT((*(l_list.begin())));
                        m_config->set_response_body_limit(l_limit);
                }
                ELIF_DIRECTIVE("secresponsebodylimitaction")
                {
                        GET_STRS("secresponsebodylimitaction missing specifier Reject|ProcessPartial");
                        if STRCASECMP((*(l_list.begin())), "Reject")
                        {
                                m_config->set_response_body_limit_action(waflz_pb::sec_config_t_limit_action_type_t_REJECT);
                        }
                        else if STRCASECMP((*(l_list.begin())), "ProcessPartial")
                        {
                                m_config->set_response_body_limit_action(waflz_pb::sec_config_t_limit_action_type_t_PROCESS_PARTIAL);
                        }
                        else
                        {
                                WAFLZ_CONFIG_ERROR_MSG("secresponsebodylimitaction missing specifier Reject|ProcessPartial");
                                return STATUS_ERROR;
                        }
                }
                ELIF_DIRECTIVE("secresponsebodymimetype")
                {
                        GET_STRS("secresponsebodymimetype missing type list");

                        // Add types...
                        for(string_list_t::iterator i_type = l_list.begin(); i_type != l_list.end(); ++i_type)
                        {
                                m_config->add_response_body_mime_type((*i_type));
                        }
                }
                ELIF_DIRECTIVE("secruleengine")
                {
                        GET_STRS("secruleengine missing specification (on|off|detectiononly");
                        if STRCASECMP((*(l_list.begin())), "ON")
                        {
                                m_config->set_rule_engine(waflz_pb::sec_config_t_engine_type_t_ON);
                        }
                        else if STRCASECMP((*(l_list.begin())), "OFF")
                        {
                                m_config->set_rule_engine(waflz_pb::sec_config_t_engine_type_t_OFF);
                        }
                        else if STRCASECMP((*(l_list.begin())), "DETECTIONONLY")
                        {
                                m_config->set_rule_engine(waflz_pb::sec_config_t_engine_type_t_DETECTION_ONLY);
                        }

                }
                ELIF_DIRECTIVE("sectmpdir")
                {
                        GET_STRS("sectmpdir missing directory string");
                        m_config->set_tmp_dir(*(l_list.begin()));
                }
                ELIF_DIRECTIVE("secdebuglog")
                {
                        GET_STRS("sectmpdir missing debug log file string");
                        m_config->set_debug_log(*(l_list.begin()));
                }
                ELIF_DIRECTIVE("secdebugloglevel")
                {
                        GET_STRS("secdebugloglevel missing size");
                        uint32_t l_level = STR2INT((*(l_list.begin())));
                        m_config->set_debug_log_level(l_level);
                }
                else
                {
                        std::string l_lowercase = l_directive;
                        std::transform(l_lowercase.begin(), l_lowercase.end(), l_lowercase.begin(), ::tolower);
                        ++(m_unimplemented_directives[l_lowercase]);
                }


        }
        //NDBG_OUTPUT("%s\n", a_line);
        // TOKENIZE line
        // Includes
        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_line(const char *a_line,
                                std::string &ao_cur_line,
                                uint32_t a_line_len)
{

        // Line index
        uint32_t i_char = 0;
        const char *l_line = a_line;


        //NDBG_OUTPUT("+=============================== LINE =================================+\n");
        //NDBG_OUTPUT("%s", l_line);
        //NDBG_OUTPUT("+----------------------------------------------------------------------+\n");

        // Scan past whitespace
        SCAN_OVER_SPACE(l_line, i_char, a_line_len);
        if(i_char == a_line_len)
        {
                return STATUS_OK;
        }

        // -------------------------------------------------
        // If first non-whitespace is comment
        // -move on
        // -------------------------------------------------
        if(*l_line == '#')
        {
                return STATUS_OK;
        }

        // -------------------------------------------------
        // Search for continuation back to front
        // -------------------------------------------------
        const char *l_line_end = a_line + a_line_len - 1;
        int32_t i_char_end = a_line_len;
        SCAN_OVER_SPACE_BKWD(l_line_end, i_char_end);
        if(i_char_end <= 0)
        {
                return STATUS_OK;
        }

        if(*l_line_end == '\\')
        {
                // Continuation -append to current line and return
                ao_cur_line.append(l_line, i_char_end - i_char - 1);
                return STATUS_OK;
        }

        // -------------------------------------------------
        // Else we have a complete line
        // -------------------------------------------------
        ao_cur_line.append(l_line, i_char_end - i_char);

        int32_t l_status;
        l_status = read_wholeline(ao_cur_line.c_str(), ao_cur_line.length());
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }

        // clear line
        ao_cur_line.clear();

        // Done...
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_file_modsec(const std::string &a_file, bool a_force)
{


        //NDBG_PRINT("FILE: %s\n", a_file.c_str());

        // ---------------------------------------
        // Check is a file
        // TODO
        // ---------------------------------------
        struct stat l_stat;
        int32_t l_status = STATUS_OK;
        l_status = stat(a_file.c_str(), &l_stat);
        if(l_status != 0)
        {
                NDBG_PRINT("Error performing stat on file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // Check if is regular file
        if(!(l_stat.st_mode & S_IFREG))
        {
                NDBG_PRINT("Error opening file: %s.  Reason: is NOT a regular file\n", a_file.c_str());
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Check for *.conf
        // TODO -skip files w/o *.conf suffix
        // ---------------------------------------
        if(!a_force)
        {
                std::string l_file_ext;
                l_file_ext = get_file_ext(a_file);
                if(l_file_ext != "conf")
                {
                        if(m_verbose)
                        {
                                NDBG_PRINT("Skiping file: %s.  Reason: Missing .conf extension.\n", a_file.c_str());
                        }
                        return STATUS_OK;
                }
        }

        // ---------------------------------------
        // Open file...
        // ---------------------------------------
        FILE * l_file;
        l_file = fopen(a_file.c_str(),"r");
        if (NULL == l_file)
        {
                NDBG_PRINT("Error opening file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // Set current state
        m_cur_file = a_file;
        m_cur_file_base = get_file_wo_path(a_file);
        m_cur_line_num = 0;
        m_cur_line_pos = 0;

        std::string l_modsec_line;
        // -----------------------------------------------------------
        // TODO -might be faster to set max size and fgets instead
        // of realloc'ing and free'ing with getline
        // MAX_READLINE_SIZE will have to be very big for modsec
        // rules -test with "wc" for max and double largest...
        // -----------------------------------------------------------
#if 0
        char l_readline[MAX_READLINE_SIZE];
        while(fgets(l_readline, sizeof(l_readline), a_file_ptr))
        {
#endif

        ssize_t l_file_line_size = 0;
        char *l_file_line = NULL;
        size_t l_unused;
        m_cur_line_num = 0;
        while((l_file_line_size = getline(&l_file_line,&l_unused,l_file)) != -1)
        {

                ++m_cur_line_num;

                // TODO strnlen -with max line length???
                if(l_file_line_size > 0)
                {
                        // For errors
                        m_cur_line = l_file_line;
                        //NDBG_PRINT("FILE: %s LINE[%d len:%d]: %s\n", m_cur_file.c_str(), m_cur_line_num, (int)l_file_line_size, m_cur_line.c_str());
                        l_status = read_line(l_file_line, l_modsec_line, (uint32_t)l_file_line_size);
                        if (STATUS_OK != l_status)
                        {
                                return STATUS_ERROR;
                        }
                }

                if(l_file_line)
                {
                        free(l_file_line);
                        l_file_line = NULL;
                }

        }

        // ---------------------------------------
        // Close file...
        // ---------------------------------------
        l_status = fclose(l_file);
        if (STATUS_OK != l_status)
        {
                NDBG_PRINT("Error performing fclose.  Reason: %s\n", strerror(errno));
                return STATUS_ERROR;
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
static void show_map(const count_map_t &a_count_map, const char *a_msg)
{
        // Dump unimplemented guys
        if(a_count_map.empty())
                return;

        NDBG_OUTPUT("+---------------------------------+----------+\n");
        NDBG_OUTPUT("| %s%-32s%s| Count    |\n",
                        ANSI_COLOR_FG_RED, a_msg, ANSI_COLOR_OFF);
        NDBG_OUTPUT("+---------------------------------+----------+\n");
        for(count_map_t::const_iterator i_str = a_count_map.begin();
                        i_str != a_count_map.end();
                        ++i_str)
        {
                NDBG_OUTPUT("| %s%-32s%s| %8d |\n",
                                ANSI_COLOR_FG_YELLOW, i_str->first.c_str(), ANSI_COLOR_OFF,
                                i_str->second);
        }
        NDBG_OUTPUT("+---------------------------------+----------+\n");
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void wafl_parser::show_status(void)
{

        // Dump unimplemented guys
        show_map(m_unimplemented_directives,"Unimplemented Directives");
        show_map(m_unimplemented_variables,"Unimplemented Variables");
        show_map(m_unimplemented_operators,"Unimplemented Operators");
        show_map(m_unimplemented_actions,"Unimplemented Actions");
        show_map(m_unimplemented_transformations,"Unimplemented Transforms");

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t get_pcre_match_list(const char *a_regex, const char *a_str, match_list_t &ao_match_list)
{

        pcre *l_re;
        const char *l_error;
        int l_erroffset;

        l_re = pcre_compile(a_regex,      // the pattern
                            PCRE_ANCHORED,// options
                            &l_error,     // for error message
                            &l_erroffset, // for error offset
                            0);           // use default character tables
        if (!l_re)
        {
                NDBG_PRINT("pcre_compile failed (offset: %d), %s\n", l_erroffset, l_error);
                return STATUS_ERROR;
        }

        uint32_t l_offset = 0;
        uint32_t l_len = strlen(a_str);
        int l_rc;
        int l_ovector[100];

        while (l_offset < l_len)
        {
                l_rc = pcre_exec(l_re,                  // Compiled pattern
                                 0,                     // Study
                                 a_str,                 // str
                                 l_len,                 // str len
                                 l_offset,              // str offset
                                 0,                     // options
                                 l_ovector,             // output vector for substr info
                                 sizeof(l_ovector));    // num elements in output vector
                if(l_rc < 0)
                {
                        break;
                }


                for(int i_match = 0; i_match < l_rc; ++i_match)
                {
                        std::string l_match;
                        l_match.assign(a_str + l_ovector[2*i_match], l_ovector[2*i_match+1] - l_ovector[2*i_match]);
                        ao_match_list.push_back(l_match);
                        //NDBG_PRINT("%2d: %.*s\n", i_match, l_ovector[2*i_match+1] - l_ovector[2*i_match], a_str + l_ovector[2*i_match]);
                }
                l_offset = l_ovector[1];
        }

        return STATUS_OK;
}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \notes:
//:   Syntax:        SecRule  VARIABLES  OPERATOR      [ACTIONS]
//:   Example Usage: SecRule  ARGS       "@rx attack"  "phase:1,log,deny,id:1"
//: ----------------------------------------------------------------------------
int32_t wafl_parser::get_modsec_rule_line(std::string &ao_str,
                                          const waflz_pb::sec_rule_t &a_secrule,
                                          const uint32_t a_indent,
                                          bool a_is_chained)
{

        std::string l_rule = "";

        // TODO -this is stupid
        if(a_indent)
        {
                l_rule += "\t";
        }

        // -----------------------------------------------------------
        // Directive
        // -----------------------------------------------------------
        l_rule += "SecRule";
        l_rule += " ";

        // -----------------------------------------------------------
        // Variables
        // -----------------------------------------------------------
        bool l_bracket_with_quotes = false;
        // Bracket TX with quotes
        if(a_secrule.variable_size() &&
           a_secrule.variable(0).has_type() &&
           (a_secrule.variable(0).type() == waflz_pb::sec_rule_t_variable_t_type_t_TX))
        {
                l_bracket_with_quotes = true;
        }

        if(l_bracket_with_quotes)
        {
                l_rule += "\"";
        }

        for(int32_t i_var = 0; i_var < a_secrule.variable_size(); ++i_var)
        {
                const waflz_pb::sec_rule_t_variable_t &l_var = a_secrule.variable(i_var);
                if(l_var.has_type())
                {

                        if(l_var.match_size() == 0)
                        {
                                if(l_var.has_is_count() && l_var.is_count())
                                {
                                        l_rule += "&";
                                }

                                // Reflect Variable name
                                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                                waflz_pb::sec_rule_t_variable_t_type_t_descriptor()->FindValueByNumber(l_var.type());
                                if(l_descriptor != NULL)
                                {
                                        // Reflect Variable name
                                        l_rule += l_descriptor->name();
                                }
                                else
                                {
                                        NDBG_PRINT("Error getting descriptor for variable type\n");
                                        return STATUS_ERROR;
                                }
                        }
                        else
                        {

                                for(int i_match = 0; i_match < l_var.match_size(); ++i_match)
                                {
                                        const waflz_pb::sec_rule_t_variable_t_match_t &l_match = l_var.match(i_match);

                                        if(l_match.has_is_negated() && l_match.is_negated())
                                        {
                                                l_rule += "!";
                                        }

                                        // Reflect Variable name
                                        const google::protobuf::EnumValueDescriptor* l_descriptor =
                                                        waflz_pb::sec_rule_t_variable_t_type_t_descriptor()->FindValueByNumber(l_var.type());
                                        if(l_descriptor != NULL)
                                        {
                                                // Reflect Variable name
                                                l_rule += l_descriptor->name();
                                        }
                                        else
                                        {
                                                NDBG_PRINT("Error getting descriptor for variable type\n");
                                                return STATUS_ERROR;
                                        }

                                        if(l_match.has_value())
                                        {
                                                // if pipe in string -quote string
                                                if(l_match.value().find('|',0) != std::string::npos)
                                                {
                                                        l_rule += ":'";
                                                        if(l_match.has_is_regex() && l_match.is_regex())
                                                        l_rule += "/";
                                                        l_rule += l_match.value();
                                                        if(l_match.has_is_regex() && l_match.is_regex())
                                                        l_rule += "/";
                                                        l_rule += "'";
                                                }
                                                else
                                                {
                                                        l_rule += ":";
                                                        if(l_match.has_is_regex() && l_match.is_regex())
                                                        {
                                                                if(l_descriptor->name() == "TX")
                                                                {
                                                                        l_rule += "'";
                                                                }
                                                                l_rule += "/";
                                                        }
                                                        l_rule += l_match.value();
                                                        if(l_match.has_is_regex() && l_match.is_regex())
                                                        {
                                                                l_rule += "/";
                                                                if(l_descriptor->name() == "TX")
                                                                {
                                                                        l_rule += "'";
                                                                }
                                                        }
                                                }
                                        }

                                        if(i_match + 1 < l_var.match_size())
                                        {
                                                l_rule += "|";
                                        }
                                }
                        }

                        if((i_var + 1) < a_secrule.variable_size() && a_secrule.variable(i_var).has_type())
                        {
                                l_rule += "|";
                        }
                }
        }

        if(l_bracket_with_quotes)
        {
                l_rule += "\"";
        }

        l_rule += " ";

        // -----------------------------------------------------------
        // Operator
        // -----------------------------------------------------------
        l_rule += "\"";
        if(a_secrule.has_operator_())
        {
                const waflz_pb::sec_rule_t_operator_t &l_operator = a_secrule.operator_();

                if(l_operator.has_type())
                {

                        // Reflect Variable name
                        const google::protobuf::EnumValueDescriptor* l_descriptor =
                                        waflz_pb::sec_rule_t_operator_t_type_t_descriptor()->FindValueByNumber(l_operator.type());
                        if(l_descriptor != NULL)
                        {
                                std::string l_type = l_descriptor->name();
                                std::transform(l_type.begin(), l_type.end(), l_type.begin(), ::tolower);
                                // Reflect Variable name
                                l_rule += "@";
                                l_rule += l_type;
                                l_rule += " ";
                        }
                        else
                        {
                                NDBG_PRINT("Error getting descriptor for variable type\n");
                                return STATUS_ERROR;
                        }
                }

                if(l_operator.has_value())
                {
                        l_rule += l_operator.value();
                }

        }
        l_rule += "\"";
        l_rule += " ";

        // -----------------------------------------------------------
        // Actions
        // -----------------------------------------------------------
        if(!a_secrule.has_action())
        {
                return STATUS_OK;
        }
        const waflz_pb::sec_action_t &l_action = a_secrule.action();

        std::string l_action_str;
        if(a_secrule.chained_rule_size() || a_is_chained)
        {
                l_action_str += "chain,";
        }

#define ADD_KV_IFE_STR(_a_key) \
        if(l_action.has_##_a_key()) \
        {\
                l_action_str += #_a_key;\
                l_action_str += ":'";\
                l_action_str += l_action._a_key();\
                l_action_str += "',";\
        }

#define ADD_KV_IFE_STR_NO_QUOTE(_a_key) \
        if(l_action.has_##_a_key()) \
        {\
                l_action_str += #_a_key;\
                l_action_str += ":";\
                l_action_str += l_action._a_key();\
                l_action_str += ",";\
        }

#define ADD_KV_IFE_BOOL(_a_key) \
        if(l_action.has_##_a_key() && l_action._a_key()) \
        {\
                l_action_str += #_a_key;\
                l_action_str += ",";\
        }

#define ADD_KV_IFE_UINT32(_a_key) \
        if(l_action.has_##_a_key()) \
        {\
                char __buf[64];\
                sprintf(__buf, "%u", l_action._a_key());\
                l_action_str += #_a_key;\
                l_action_str += ":";\
                l_action_str += __buf;\
                l_action_str += ",";\
        }

#define ADD_KV_IFE_UINT32_QUOTE(_a_key) \
        if(l_action.has_##_a_key()) \
        {\
                char __buf[64];\
                sprintf(__buf, "%u", l_action._a_key());\
                l_action_str += #_a_key;\
                l_action_str += ":'";\
                l_action_str += __buf;\
                l_action_str += "',";\
        }

        ADD_KV_IFE_UINT32(phase);
        // action_type
        if(l_action.has_action_type())
        {
                // Reflect Variable name
                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                waflz_pb::sec_action_t_action_type_t_descriptor()->FindValueByNumber(l_action.action_type());
                if(l_descriptor != NULL)
                {
                        std::string l_action_type = l_descriptor->name();
                        std::transform(l_action_type.begin(), l_action_type.end(), l_action_type.begin(), ::tolower);
                        l_action_str += l_action_type;
                        l_action_str += ",";
                }
                else
                {
                        NDBG_PRINT("Error getting descriptor for action type\n");
                        return STATUS_ERROR;
                }
        }

        ADD_KV_IFE_STR(rev);
        ADD_KV_IFE_STR(ver);
        ADD_KV_IFE_STR(maturity);
        ADD_KV_IFE_STR(accuracy);

        for(int32_t i_ctl = 0; i_ctl < l_action.ctl_size(); ++i_ctl)
        {
                l_action_str += "ctl";
                l_action_str += ":";
                l_action_str += l_action.ctl(i_ctl);
                l_action_str += ",";
        }

        if(l_action.has_multimatch() && l_action.multimatch()) \
        {\
                l_action_str += "multiMatch";\
                l_action_str += ",";\
        }

        for(int32_t i_tx = 0; i_tx < l_action.t_size(); ++i_tx)
        {
                l_action_str += "t";
                l_action_str += ":";

                // Reflect transformation name
                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                waflz_pb::sec_action_t_transformation_type_t_descriptor()->FindValueByNumber(l_action.t(i_tx));
                if(l_descriptor != NULL)
                {
                        std::string l_t_type = l_descriptor->name();
                        std::transform(l_t_type.begin(), l_t_type.end(), l_t_type.begin(), ::tolower);

                        // Camel case conversion...
                        if(l_t_type == "urldecodeuni") l_t_type = "urlDecodeUni";
                        else if(l_t_type == "htmlentitydecode") l_t_type = "htmlEntityDecode";
                        else if(l_t_type == "jsdecode") l_t_type = "jsDecode";
                        else if(l_t_type == "cssdecode") l_t_type = "cssDecode";
                        else if(l_t_type == "htmlentitydecode") l_t_type = "htmlEntityDecode";
                        else if(l_t_type == "normalizepath") l_t_type = "normalisePath";

                        l_action_str += l_t_type;
                        l_action_str += ",";
                }
                else
                {
                        NDBG_PRINT("Error getting descriptor for action type\n");
                        return STATUS_ERROR;
                }
        }

        for(int32_t i_setvar = 0; i_setvar < l_action.setvar_size(); ++i_setvar)
        {
                l_action_str += "setvar";
                l_action_str += ":'";
                l_action_str += l_action.setvar(i_setvar);
                l_action_str += "',";
        }

        ADD_KV_IFE_BOOL(capture);
        ADD_KV_IFE_STR(logdata);
        ADD_KV_IFE_UINT32_QUOTE(severity);
        ADD_KV_IFE_STR_NO_QUOTE(id);
        ADD_KV_IFE_STR(msg);
        ADD_KV_IFE_BOOL(nolog);
        ADD_KV_IFE_BOOL(log);
        ADD_KV_IFE_BOOL(noauditlog);
        ADD_KV_IFE_BOOL(auditlog);
        ADD_KV_IFE_STR(initcol);
        ADD_KV_IFE_STR(status);
        ADD_KV_IFE_UINT32_QUOTE(skip);
        ADD_KV_IFE_BOOL(sanitisematched);

        for(int32_t i_tag = 0; i_tag < l_action.tag_size(); ++i_tag)
        {
                l_action_str += "tag";
                l_action_str += ":'";
                l_action_str += l_action.tag(i_tag);
                l_action_str += "',";
        }

        ADD_KV_IFE_STR_NO_QUOTE(skipafter);

        if(!l_action_str.empty())
        {

                // Chop last comma
                if(l_action_str[l_action_str.length() - 1] == ',')
                {
                        l_action_str = l_action_str.substr(0, l_action_str.size()-1);
                }

                l_rule += "\"";
                l_rule += l_action_str;
                l_rule += "\"";
        }

        // Chop last comma
        if(l_rule[l_rule.length() - 1] == ',')
        {
                l_rule = l_rule.substr(0, l_rule.size()-1);
        }

        // Assign to output
        ao_str = l_rule;
        ao_str += '\n';

        // -----------------------------------------------------------
        // chain....
        // -----------------------------------------------------------
        // For rule in sec_config_t append
        for(int32_t i_chained_rule = 0; i_chained_rule < a_secrule.chained_rule_size(); ++i_chained_rule)
        {
                int32_t l_status;
                bool l_chained = false;
                if(i_chained_rule + 1 < a_secrule.chained_rule_size())
                {
                        l_chained = true;
                }
                l_status = append_modsec_rule(ao_str,
                                              a_secrule.chained_rule(i_chained_rule),
                                              MODSECURITY_RULE_INDENT_SIZE,
                                              l_chained);
                if(l_status != STATUS_OK)
                {
                        return STATUS_ERROR;
                }
        }

        //NDBG_PRINT("%s\n", ao_str.c_str());

        return STATUS_OK;

}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::get_modsec_config_str(waflz_pb::sec_config_t *m_config, std::string &ao_str)
{

        // -------------------------------------------------
        // Create ordered map of rules to process in order
        // by id
        // -------------------------------------------------
        typedef std::map<uint64_t, const waflz_pb::sec_rule_t *> order_rule_map_t;
        order_rule_map_t l_order_rule_map;
        for(int32_t i_rule = 0; i_rule < m_config->sec_rule_size(); ++i_rule)
        {
                const waflz_pb::sec_rule_t& l_rule = m_config->sec_rule(i_rule);
                if(l_rule.has_order())
                {
                        l_order_rule_map[l_rule.order()] = &(m_config->sec_rule(i_rule));
                }
        }

        // -------------------------------------------------
        // Loop through rules
        // -------------------------------------------------
        for(order_rule_map_t::iterator i_rule = l_order_rule_map.begin(); i_rule != l_order_rule_map.end(); ++i_rule)
        {
                append_modsec_rule(ao_str, *(i_rule->second), 0, false);
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::append_modsec_rule(std::string &ao_str,
                                         const waflz_pb::sec_rule_t &a_secrule,
                                         const uint32_t a_indent,
                                         bool a_is_chained)
{

        std::string l_rule;
        int32_t l_status;
        l_status = get_modsec_rule_line(l_rule, a_secrule, a_indent, a_is_chained);
        if(l_status != STATUS_OK)
        {
                return STATUS_ERROR;
        }
        ao_str += l_rule;

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_file_pbuf(const std::string &a_file, bool a_force)
{

        // ---------------------------------------
        // Check is a file
        // TODO
        // ---------------------------------------
        struct stat l_stat;
        int32_t l_status = STATUS_OK;
        l_status = stat(a_file.c_str(), &l_stat);
        if(l_status != 0)
        {
                NDBG_PRINT("Error performing stat on file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // Check if is regular file
        if(!(l_stat.st_mode & S_IFREG))
        {
                NDBG_PRINT("Error opening file: %s.  Reason: is NOT a regular file\n", a_file.c_str());
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Check for *.conf
        // TODO -skip files w/o *.pbuf suffix
        // ---------------------------------------
        if(!a_force)
        {
                std::string l_file_ext;
                l_file_ext = get_file_ext(a_file);
                if(l_file_ext != "pbuf")
                {
                        if(m_verbose)
                        {
                                NDBG_PRINT("Skiping file: %s.  Reason: Missing .conf extension.\n", a_file.c_str());
                        }
                        return STATUS_OK;
                }
        }

        // ---------------------------------------
        // Open file...
        // ---------------------------------------
        FILE * l_file;
        l_file = fopen(a_file.c_str(),"r");
        if (NULL == l_file)
        {
                NDBG_PRINT("Error opening file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Read in file...
        // ---------------------------------------
        int32_t l_size = l_stat.st_size;
        int32_t l_read_size;
        char *l_buf = (char *)malloc(sizeof(char)*l_size);
        l_read_size = fread(l_buf, 1, l_size, l_file);
        if(l_read_size != l_size)
        {
                NDBG_PRINT("Error performing fread.  Reason: %s [%d:%d]\n", strerror(errno), l_read_size, l_size);
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Parse
        // ---------------------------------------
        bool l_parse_status;
        m_config->Clear();
        l_parse_status = m_config->ParseFromArray(l_buf, l_size);
        if(!l_parse_status)
        {
                NDBG_PRINT("Error performing ParseFromArray\n");
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Close file...
        // ---------------------------------------
        l_status = fclose(l_file);
        if (STATUS_OK != l_status)
        {
                NDBG_PRINT("Error performing fclose.  Reason: %s\n", strerror(errno));
                return STATUS_ERROR;
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_file_json(const std::string &a_file, bool a_force)
{

        // ---------------------------------------
        // Check is a file
        // TODO
        // ---------------------------------------
        struct stat l_stat;
        int32_t l_status = STATUS_OK;
        l_status = stat(a_file.c_str(), &l_stat);
        if(l_status != 0)
        {
                NDBG_PRINT("Error performing stat on file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // Check if is regular file
        if(!(l_stat.st_mode & S_IFREG))
        {
                NDBG_PRINT("Error opening file: %s.  Reason: is NOT a regular file\n", a_file.c_str());
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Check for *.conf
        // TODO -skip files w/o *.pbuf suffix
        // ---------------------------------------
        if(!a_force)
        {
                std::string l_file_ext;
                l_file_ext = get_file_ext(a_file);
                if(l_file_ext != "pbuf")
                {
                        if(m_verbose)
                        {
                                NDBG_PRINT("Skiping file: %s.  Reason: Missing .conf extension.\n", a_file.c_str());
                        }
                        return STATUS_OK;
                }
        }

        // ---------------------------------------
        // Open file...
        // ---------------------------------------
        FILE * l_file;
        l_file = fopen(a_file.c_str(),"r");
        if (NULL == l_file)
        {
                NDBG_PRINT("Error opening file: %s.  Reason: %s\n", a_file.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Read in file...
        // ---------------------------------------
        int32_t l_size = l_stat.st_size;
        int32_t l_read_size;
        char *l_buf = (char *)malloc(sizeof(char)*l_size);
        l_read_size = fread(l_buf, 1, l_size, l_file);
        if(l_read_size != l_size)
        {
                NDBG_PRINT("Error performing fread.  Reason: %s [%d:%d]\n", strerror(errno), l_read_size, l_size);
                return STATUS_ERROR;
        }

        // ---------------------------------------
        // Parse
        // ---------------------------------------
        try
        {
                ns_jspb::update_from_json(*m_config, l_buf, l_size);
        }
        catch(int e)
        {
                NDBG_PRINT("Error -json_protobuf::convert_to_json threw\n");
                return STATUS_ERROR;
        }


        // ---------------------------------------
        // Close file...
        // ---------------------------------------
        l_status = fclose(l_file);
        if (STATUS_OK != l_status)
        {
                NDBG_PRINT("Error performing fclose.  Reason: %s\n", strerror(errno));
                return STATUS_ERROR;
        }

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_directory(format_t a_format, const std::string &a_directory)
{
        // -------------------------------------------------
        // Walk through directory (no recursion)
        // -------------------------------------------------
        typedef std::set<std::string> file_set_t;
        file_set_t l_file_set;

        // Scan directory for existing
        DIR *l_dir_ptr;
        struct dirent *l_dirent;
        l_dir_ptr = opendir(a_directory.c_str());
        if (l_dir_ptr != NULL) {
                // if no error

                while ((l_dirent =
                        readdir(l_dir_ptr)) != NULL) {
                        // While files

                        // Get extension
                        std::string l_filename(a_directory);
                        l_filename += "/";
                        l_filename += l_dirent->d_name;

                        // Skip directories
                        struct stat l_stat;
                        int32_t l_status = STATUS_OK;
                        l_status = stat(l_filename.c_str(), &l_stat);
                        if(l_status != 0)
                        {
                                NDBG_PRINT("Error performing stat on file: %s.  Reason: %s\n", l_filename.c_str(), strerror(errno));
                                return STATUS_ERROR;
                        }

                        // Check if is directory
                        if(l_stat.st_mode & S_IFDIR)
                        {
                                continue;
                        }

                        l_file_set.insert(l_filename);

                }

                closedir(l_dir_ptr);

        }
        else {

                NDBG_PRINT("Failed to open directory: %s.  Reason: %s\n", a_directory.c_str(), strerror(errno));
                return STATUS_ERROR;

        }

        // Read every file
        for(file_set_t::const_iterator i_file = l_file_set.begin(); i_file != l_file_set.end(); ++i_file)
        {
                int32_t l_status;
                l_status = read_file(a_format, *i_file, false);
                if(l_status != STATUS_OK) {

                        // Fail or continue???
                        // Continuing for now...
                        NDBG_PRINT("Error performing read_file: %s\n", i_file->c_str());
                        //return STATUS_ERROR;
                }
        }


        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::read_file(format_t a_format, const std::string &a_file, bool a_force)
{
        int l_status;
        switch(a_format)
        {
        case format_t::MODSECURITY:
        {
                l_status = read_file_modsec(a_file, a_force);
                return l_status;
        }
        case format_t::JSON:
        {
                l_status = read_file_json(a_file, a_force);
                return l_status;
        }
        case format_t::PROTOBUF:
        {
                l_status = read_file_pbuf(a_file, a_force);
                return l_status;
        }
        default:
        {
                // TODO Add message
                return STATUS_ERROR;
        }
        }

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_parser::parse_config(format_t a_format,
                                  const std::string &a_path,
                                  waflz_pb::sec_config_t *a_config)

{

        m_config = a_config;

        // Stat file to see if is directory or file
        struct stat l_stat;
        int32_t l_status = STATUS_OK;
        l_status = stat(a_path.c_str(), &l_stat);
        if(l_status != 0)
        {
                NDBG_PRINT("Error performing stat on file: %s.  Reason: %s\n", a_path.c_str(), strerror(errno));
                return STATUS_ERROR;
        }

        // Check if is directory
        if(l_stat.st_mode & S_IFDIR)
        {
                // Bail out for pbuf or json inputs
                if((a_format == format_t::PROTOBUF) || (a_format == format_t::JSON))
                {
                        NDBG_PRINT("Error directories unsupported for json or pbuf input types.\n");
                        return STATUS_ERROR;
                }

                int32_t l_retval = STATUS_OK;
                l_retval = read_directory(a_format, a_path);
                if(l_retval != STATUS_OK)
                {
                        return STATUS_ERROR;
                }
        // File
        } else if((l_stat.st_mode & S_IFREG) || (l_stat.st_mode & S_IFLNK))
        {
                int32_t l_retval = STATUS_OK;
                l_retval = read_file(a_format, a_path, true);
                if(l_retval != STATUS_OK)
                {
                        return STATUS_ERROR;
                }
        }

        return STATUS_OK;
}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl_parser::wafl_parser(void):
        m_verbose(false),
        m_color(false),
        m_cur_file(),
        m_cur_file_base(),
        m_cur_line(),
        m_cur_line_num(0),
        m_cur_line_pos(0),
        m_cur_parent_rule(NULL),
        m_config(NULL),
        m_unimplemented_directives(),
        m_unimplemented_variables(),
        m_unimplemented_operators(),
        m_unimplemented_actions(),
        m_unimplemented_transformations()
{
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl_parser::~wafl_parser()
{
}


//: ----------------------------------------------------------------------------
//: Class variables
//: ----------------------------------------------------------------------------
