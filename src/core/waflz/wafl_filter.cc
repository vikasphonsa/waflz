//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl_filter.cc
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
#include "wafl_filter.h"
#include "waflz.pb.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

// File stat
#include <sys/stat.h>
#include <unistd.h>

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Fwd decl's
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
std::string wafl_filter::get_tx_setvar(const std::string &a_var)
{
        std::string l_retval = "";

        //"tx.ccdata=%{tx.1}"
        // Get string after tx up to =
        if(strncasecmp(a_var.c_str(), "tx.", strlen("tx.")) == 0)
        {
                size_t l_equal_offset;
                l_equal_offset = a_var.find("=", strlen("tx."));
                l_retval.assign(a_var.c_str() + strlen("tx."), l_equal_offset - strlen("tx."));
        }

        return l_retval;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool wafl_filter::has_setvar(const waflz_pb::sec_rule_t &a_rule)
{
        if(a_rule.variable_size())
        {
                for(int32_t i_var = 0; i_var < a_rule.variable_size(); ++i_var)
                {
                        if(a_rule.variable(i_var).has_type() &&
                           a_rule.variable(i_var).type() == waflz_pb::sec_rule_t_variable_t_type_t_TX)
                        {
                                for(int i_match = 0; i_match < a_rule.variable(i_var).match_size(); ++i_match)
                                {
                                        if(a_rule.variable(i_var).match(i_match).has_value() &&
                                           !a_rule.variable(i_var).match(i_match).value().empty())
                                        {
                                                //NDBG_PRINT("CHECKING SETVAR: %s\n", a_rule.variable(i_var).value().c_str());
                                                std::string l_var = a_rule.variable(i_var).match(i_match).value();
                                                // Loop through
                                                for(rule_var_set_t::const_iterator i_v = m_var_set.begin();
                                                                i_v != m_var_set.end();
                                                                ++i_v)
                                                {
                                                        if(strcasecmp((*i_v).c_str(), l_var.c_str()) == 0)
                                                        {
                                                                return true;
                                                        }
                                                }

                                        }
                                }
                        }
                }
        }
        return false;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void wafl_filter::push_setvar(const waflz_pb::sec_rule_t &a_rule)
{

        if(!a_rule.has_action())
        {
                return;
        }
        const waflz_pb::sec_action_t &l_action = a_rule.action();

        // Add any setvar's
        // check rule and chained rules
        if(l_action.setvar_size())
        {
                for(int32_t i_var = 0; i_var < l_action.setvar_size(); ++i_var)
                {
                        //NDBG_PRINT("PUSHING SETVAR: %s\n", a_rule.setvar(i_var).c_str());

                        std::string l_var;
                        l_var = get_tx_setvar(l_action.setvar(i_var));
                        if(!l_var.empty())
                        {
                                std::transform(l_var.begin(), l_var.end(), l_var.begin(), ::tolower);
                                //NDBG_PRINT("ADDING SETVAR: %s\n", l_var.c_str());
                                m_var_set.insert(l_var);
                        }
                }
        }
        if(a_rule.chained_rule_size())
        {
                for(int32_t i_cr = 0; i_cr < a_rule.chained_rule_size(); ++i_cr)
                {
                        if(a_rule.chained_rule(i_cr).action().setvar_size())
                        {
                                for(int32_t i_var = 0; i_var < l_action.setvar_size(); ++i_var)
                                {
                                        //NDBG_PRINT("PUSHING SETVAR: %s\n", a_rule.setvar(i_var).c_str());

                                        std::string l_var;
                                        l_var = get_tx_setvar(l_action.setvar(i_var));
                                        if(!l_var.empty())
                                        {
                                                std::transform(l_var.begin(), l_var.end(), l_var.begin(), ::tolower);
                                                //NDBG_PRINT("ADDING SETVAR: %s\n", l_var.c_str());
                                                m_var_set.insert(l_var);
                                        }
                                }
                        }
                }
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool wafl_filter::match_msg(const waflz_pb::sec_rule_t &a_rule, const std::string &a_match)
{
        bool l_match = false;

        if(!a_rule.has_action())
        {
                return false;
        }
        const waflz_pb::sec_action_t &l_action = a_rule.action();

        if(l_action.has_msg())
        {
                // Check message
                std::string l_match_lower = a_match;
                std::transform(l_match_lower.begin(), l_match_lower.end(), l_match_lower.begin(), ::tolower);

                // Convert msg lower
                std::string l_msg = l_action.msg();
                std::transform(l_msg.begin(), l_msg.end(), l_msg.begin(), ::tolower);

                if(l_msg.find(l_match_lower) != std::string::npos)
                {
                        //NDBG_PRINT("%sMATCH%s: %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, a_rule.ShortDebugString().c_str());
                        l_match = true;
                }
        }
        return l_match;
};

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool wafl_filter::match_file(const waflz_pb::sec_rule_t &a_rule, const std::string &a_match)
{
        bool l_match = false;

        if(!a_rule.has_action())
        {
                return false;
        }
        const waflz_pb::sec_action_t &l_action = a_rule.action();

        if(l_action.has_file())
        {
                // Check message
                std::string l_match_lower = a_match;
                std::transform(l_match_lower.begin(), l_match_lower.end(), l_match_lower.begin(), ::tolower);

                // Convert msg lower
                std::string l_msg = l_action.file();
                std::transform(l_msg.begin(), l_msg.end(), l_msg.begin(), ::tolower);

                if(l_msg.find(l_match_lower) != std::string::npos)
                {
                        //NDBG_PRINT("%sMATCH%s: %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, a_rule.ShortDebugString().c_str());
                        l_match = true;
                }
        }
        return l_match;
};

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void wafl_filter::add_match(filter_t a_filter, std::string &a_match, bool a_is_negated)
{
        match_t l_match;
        l_match.m_type = a_filter;
        l_match.m_match = a_match;
        l_match.m_is_negated = a_is_negated;
        m_match_list.push_back(l_match);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool wafl_filter::match(const waflz_pb::sec_rule_t &a_rule)
{
        bool l_match = false;
        for(match_list_t::const_iterator i_match = m_match_list.begin();
            i_match != m_match_list.end();
            ++i_match)
        {
                bool l_match_tmp = false;
                switch(i_match->m_type)
                {
                case MSG:
                {
                        l_match_tmp = match_msg(a_rule, i_match->m_match);
                        break;
                }
                case FILE:
                {
                        l_match_tmp = match_file(a_rule, i_match->m_match);
                        break;
                }
                default:
                {
                        break;
                }
                }

                // If is negated and a match -bail out
                if(i_match->m_is_negated)
                {
                        if(l_match_tmp)
                        {
                                return false;
                        }
                }
                else
                {
                        l_match = l_match_tmp;
                }
        }

        // Check for setvar's
        if(has_setvar(a_rule))
        {
                return true;
        }

        if(l_match)
        {
                push_setvar(a_rule);
        }

        return l_match;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_filter::init_with_line(const std::string &a_line)
{

        //NDBG_PRINT("init_with_line: %s\n",a_line.c_str());

        // Check if already is initd
        if(m_is_initd)
                return STATUS_OK;

        m_filter_json = new Json::Value(Json::objectValue);

        Json::Reader l_reader;
        bool l_result = l_reader.parse(a_line, *m_filter_json);
        if(!l_result)
        {
                NDBG_PRINT("Error parsing json line: %s\n", a_line.c_str());
                return STATUS_ERROR;
        }

        //NDBG_PRINT("Filtering size: %u\n",a_filter.m_filter_json->size());
        // ---------------------------------------
        // Add filters...
        // ---------------------------------------
        const Json::Value &l_json = *m_filter_json;
        const Json::Value l_sec_rule = l_json["sec_rule"];
        if(l_sec_rule != Json::nullValue)
        {
                const Json::Value l_msg_filter = l_json["sec_rule"]["msg"];
                if(l_msg_filter != Json::nullValue)
                {
                        bool l_is_negated = false;
                        std::string l_match = l_msg_filter.asCString();
                        if(l_match[0] == '!')
                        {
                                std::string l_tmp = l_match;
                                l_match.assign(l_tmp.c_str() + 1, l_tmp.length() - 1);
                                l_is_negated = true;
                        }
                        add_match(MSG, l_match, l_is_negated);
                }
                const Json::Value l_file_filter = l_json["sec_rule"]["file"];
                if(l_file_filter != Json::nullValue)
                {
                        bool l_is_negated = false;
                        std::string l_match = l_file_filter.asCString();
                        if(l_match[0] == '!')
                        {
                                std::string l_tmp = l_match;
                                l_match.assign(l_tmp.c_str() + 1, l_tmp.length() - 1);
                                l_is_negated = true;
                        }
                        add_match(FILE, l_match, l_is_negated);
                }
        }

        m_is_initd = true;
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl_filter::init_with_file(const std::string &a_file)
{
        // Check if already is initd
        if(m_is_initd)
                return STATUS_OK;

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
        // Open file...
        // ---------------------------------------
        ::FILE *l_file;
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
        // Read in file
        std::string l_line;
        l_status = init_with_line(l_line);
        if(l_status != STATUS_OK)
        {
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


        m_is_initd = true;
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl_filter::wafl_filter(void):
        m_filter_json(NULL),
        m_is_initd(false),
        m_var_set(),
        m_match_list()
{
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl_filter::~wafl_filter()
{
        if(m_filter_json)
        {
                delete m_filter_json;
                m_filter_json = NULL;
        }
}


//: ----------------------------------------------------------------------------
//: Class variables
//: ----------------------------------------------------------------------------
