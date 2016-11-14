//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl.cc
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
#include "util.h"
#include "wafl.h"
#include "wafl_filter.h"
#include "wafl_parser.h"
#include "jspb.h"
#include <google/protobuf/descriptor.h>
#include <errno.h>
#include <string.h>
#include <set>
#include <regex.h>

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
int32_t wafl::filter(wafl_filter &a_filter)
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

        waflz_pb::sec_config_t *l_fconfig = new waflz_pb::sec_config_t();

        // -------------------------------------------------
        // Loop through rules
        // -------------------------------------------------
        for(order_rule_map_t::iterator i_rule = l_order_rule_map.begin(); i_rule != l_order_rule_map.end(); ++i_rule)
        {
                if(a_filter.match(*(i_rule->second)))
                {
                        //NDBG_PRINT("%sMATCH%s: ADDING\n", ANSI_COLOR_BG_MAGENTA, ANSI_COLOR_OFF);
                        waflz_pb::sec_rule_t* l_add_rule = l_fconfig->add_sec_rule();
                        l_add_rule->CopyFrom(*(i_rule->second));
                }
        }

        // Swap
        if(m_config)
        {
                delete m_config;
                m_config = NULL;
        }

        m_config = l_fconfig;

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
#define WAFLZ_FG_COLOR_LIST_LENGTH 6
const char g_color_off[16] = ANSI_COLOR_OFF;
const char g_color_fg_list[WAFLZ_FG_COLOR_LIST_LENGTH][16] = {

                ANSI_COLOR_FG_RED,
                ANSI_COLOR_FG_GREEN,
                ANSI_COLOR_FG_YELLOW,
                ANSI_COLOR_FG_BLUE,
                ANSI_COLOR_FG_MAGENTA,
                ANSI_COLOR_FG_CYAN
};

typedef struct {

        char m_token[32];
        char m_color[32];

} token_color_t;

const token_color_t g_token_color_map[] = {

        {"id:",   ANSI_COLOR_FG_YELLOW},
        {"msg:",  ANSI_COLOR_FG_GREEN},
        {"file:", ANSI_COLOR_FG_CYAN},

};

#define TOKEN_COLOR_MAP_LEN (sizeof(g_token_color_map)/sizeof(g_token_color_map[0]))

regex_t g_regex;
bool g_regex_initialized = false;
static int colorize_string(std::string &ao_string)
{

        int reti = 0;
        regmatch_t pmatch[1];
        uint32_t l_last_offt = 0;

        if(!g_regex_initialized)
        {
                reti = regcomp(&g_regex, "[[:space:]][A-Za-z0-9_]+:", REG_EXTENDED);
                if( reti ){ fprintf(stderr, "Could not compile regex\n"); exit(1); }
                g_regex_initialized = true;
        }

        const char *l_str_ptr = ao_string.data();

        while(regexec(&g_regex, l_str_ptr, 1, pmatch, 0) == 0)
        {
                //printf("Match reti = %d --pmatch = %d --> %d\n", reti, pmatch[0].rm_so, pmatch[0].rm_eo);
                uint32_t l_match_start = pmatch[0].rm_so + l_last_offt;
                uint32_t l_match_end = pmatch[0].rm_eo + l_last_offt;

                const char *l_search_ptr = ao_string.data() + l_match_start + 1 + strlen(ANSI_COLOR_FG_BLUE);

                ao_string.insert(l_match_start, ANSI_COLOR_FG_BLUE);
                ao_string.insert((l_match_end + strlen(ANSI_COLOR_FG_BLUE)), ANSI_COLOR_OFF);

                // Check for symbol and name
                uint32_t i_token;
                for(i_token = 0; i_token < TOKEN_COLOR_MAP_LEN; ++i_token)
                {
                        if(strncmp(l_search_ptr,
                                   g_token_color_map[i_token].m_token,
                                   strlen(g_token_color_map[i_token].m_token)) == 0)
                        {
                                ao_string.insert((l_match_end + strlen(ANSI_COLOR_FG_BLUE) + strlen(ANSI_COLOR_OFF)),
                                                g_token_color_map[i_token].m_color);
                                l_last_offt+= strlen(g_token_color_map[i_token].m_color);
                        }
                }
                l_last_offt+= (pmatch[0].rm_eo) + strlen(ANSI_COLOR_BG_BLUE) + strlen(ANSI_COLOR_OFF);
                l_str_ptr = (char *)(ao_string.data() + l_last_offt);
        }

        return 0;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void wafl::show(void)
{
        std::string l_config = m_config->DebugString();
        colorize_string(l_config);
        NDBG_OUTPUT("%s\n", l_config.c_str());
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void wafl::show_status(void)
{
        m_parser.show_status();
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
void wafl::show_debug(void)
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
int32_t wafl::get_str(std::string &ao_str, wafl_parser::format_t a_format)
{
        bool l_status;


        switch(a_format)
        {
        // ---------------------------------------
        // Protobuf
        // ---------------------------------------
        case wafl_parser::PROTOBUF:
        {
                l_status = m_config->SerializeToString(&ao_str);
                if(!l_status)
                {
                        return STATUS_ERROR;
                }
                else
                {
                        return STATUS_OK;
                }
                break;
        }
        // ---------------------------------------
        // json
        // ---------------------------------------
        case wafl_parser::JSON:
        {
                // convert protobuf message to JsonCpp object
                try
                {
                        ns_jspb::convert_to_json(ao_str, *m_config);
                }
                catch(int e)
                {
                        NDBG_PRINT("Error -json_protobuf::convert_to_json threw\n");
                        return STATUS_ERROR;
                }
                return STATUS_OK;
        }
        // ---------------------------------------
        // modsecurity
        // ---------------------------------------
        case wafl_parser::MODSECURITY:
        {
                wafl_parser l_parser;
                l_status = l_parser.get_modsec_config_str(m_config, ao_str);
                if(l_status != STATUS_OK)
                {
                        //NDBG_PRINT("Error performing get_modsec_config_str\n");
                        return STATUS_ERROR;
                }
                else
                {
                        return STATUS_OK;
                }
                break;
        }
        default:
        {
                NDBG_PRINT("Error -unrecognized format specification[%d]\n", a_format);
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
int32_t wafl::get_last_matched_rule_str(std::string &ao_str)
{

        if(!m_last_matched_secrule)
        {
                return STATUS_ERROR;
        }

        // convert protobuf message to JsonCpp object
        try
        {
                ns_jspb::convert_to_json(ao_str, *m_last_matched_secrule);
        }
        catch(int e)
        {
                NDBG_PRINT("Error -json_protobuf::convert_to_json threw\n");
                return STATUS_ERROR;
        }


        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl::init(wafl_parser::format_t a_format, const std::string &a_path)
{
        // Check if already is initd
        if(m_is_initd)
                return STATUS_OK;

        int32_t l_status;
        l_status = m_parser.parse_config(a_format, a_path, m_config);
        if(l_status != STATUS_OK)
        {
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
void wafl::preprocess(void)
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
        for(order_rule_map_t::const_iterator i_rule = l_order_rule_map.begin(); i_rule != l_order_rule_map.end(); ++i_rule)
        {

                // Get action
                if(!i_rule->second->has_action())
                {
                        continue;
                }

                const waflz_pb::sec_action_t &l_action = i_rule->second->action();

                //append_modsec_rule(ao_str, *(i_rule->second), 0, false);
                if(!l_action.has_phase())
                {
                        continue;
                }

                uint32_t l_phase = l_action.phase();

                switch(l_phase)
                {
                case MODSECURITY_RULE_PHASE_REQUEST_HEADERS:
                {
                        m_phase_request_headers.push_back(i_rule->second);
                        break;
                }
                case MODSECURITY_RULE_PHASE_REQUEST_BODY:
                {
                        m_phase_request_body.push_back(i_rule->second);
                        break;
                }
                case MODSECURITY_RULE_PHASE_RESPONSE_HEADERS:
                {
                        m_phase_response_headers.push_back(i_rule->second);
                        break;
                }
                case MODSECURITY_RULE_PHASE_RESPONSE_BODY:
                {
                        m_phase_response_body.push_back(i_rule->second);
                        break;
                }
                case MODSECURITY_RULE_PHASE_LOGGING:
                {
                        m_phase_logging.push_back(i_rule->second);
                        break;
                }
                default:
                {
                        // Do nuttin...
                        break;
                }
                }

        }

        // Loop through all rules adding to respective phases


        m_is_preprocessed = true;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \notes:
//:     TODO:
//:     1. Add support for caching ref_list based on req
//: ----------------------------------------------------------------------------
int32_t wafl::get_vars(const waflz_pb::sec_rule_t_variable_t &a_var,
                       var_ref_list_t *ao_var_ref_list,
                       const http_request &a_req)
{

        if(!a_var.has_type())
                return STATUS_OK;

        // TODO FIX!!!!!!
#if 0
        // Reflect Variable name
        const google::protobuf::EnumValueDescriptor* l_descriptor =
                        waflz_pb::sec_rule_t_variable_t_type_t_descriptor()->FindValueByNumber(a_var.type());

        if(a_var.is_negated())
        {
                if(a_var.has_match())
                {
                        NDBG_PRINT("%s__VAR__%s: %s:%s\n", ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, l_descriptor->name().c_str(), a_var.match().c_str());
                }
                else
                {
                        NDBG_PRINT("%s__VAR__%s: %s\n", ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, l_descriptor->name().c_str());
                }
        }
        else
        {
                if(a_var.has_match())
                {
                        NDBG_PRINT("%s__VAR__%s: %s:%s\n", ANSI_COLOR_FG_RED, ANSI_COLOR_OFF, l_descriptor->name().c_str(), a_var.match().c_str());
                }
                else
                {
                        NDBG_PRINT("%s__VAR__%s: %s\n", ANSI_COLOR_FG_RED, ANSI_COLOR_OFF, l_descriptor->name().c_str());
                }
        }
#endif

        switch(a_var.type())
        {
        case waflz_pb::sec_rule_t_variable_t_type_t_ARGS:
        {
                const kv_list_map_t &l_query = const_cast <http_request &>(a_req).get_uri_decoded_query();

                for(kv_list_map_t::const_iterator i_q = l_query.begin();
                    i_q != l_query.end();
                    ++i_q)
                {

                        // TODO FIX!!!
#if 0
                        // Filter by match...
                        if(a_var.has_match())
                        {
                                if(a_var.has_is_negated() && a_var.is_negated())
                                {
                                        if(strstr(i_q->first.c_str(), a_var.match().c_str()) != NULL)
                                        {
                                                continue;
                                        }
                                }
                                else
                                {
                                        if(strstr(i_q->first.c_str(), a_var.match().c_str()) == NULL)
                                        {
                                                continue;
                                        }
                                }
                        }
#endif

                        if(!i_q->second.empty())
                        {
                                for(value_list_t::const_iterator i_v = i_q->second.begin();
                                    i_v != i_q->second.end();
                                    ++i_v)
                                {
                                        if(!(i_v->empty()))
                                        {
                                                var_ref_t l_var_ref;

                                                l_var_ref.m_key = i_q->first.data();
                                                l_var_ref.m_data = i_v->data();

                                                //NDBG_PRINT("%sMATCH%s: match: %s\n",
                                                //                ANSI_COLOR_FG_BLUE, ANSI_COLOR_OFF, l_var_ref.m_data);
                                                ao_var_ref_list->push_back(l_var_ref);
                                        }
                                }
                        }
                }
                break;
        }
        case waflz_pb::sec_rule_t_variable_t_type_t_ARGS_NAMES:
        {
                const kv_list_map_t &l_query = const_cast <http_request &>(a_req).get_uri_decoded_query();

                for(kv_list_map_t::const_iterator i_q = l_query.begin();
                    i_q != l_query.end();
                    ++i_q)
                {

                        // TODO FIX!!!
#if 0
                        // Filter by match...
                        if(a_var.has_match())
                        {
                                if(a_var.has_is_negated() && a_var.is_negated())
                                {
                                        if(strstr(i_q->first.c_str(), a_var.match().c_str()) != NULL)
                                        {
                                                continue;
                                        }
                                }
                                else
                                {
                                        if(strstr(i_q->first.c_str(), a_var.match().c_str()) == NULL)
                                        {
                                                continue;
                                        }
                                }
                        }
#endif

                        if(!i_q->first.empty())
                        {
                                var_ref_t l_var_ref;

                                l_var_ref.m_key = i_q->first.data();
                                l_var_ref.m_data = i_q->first.data();

                                //NDBG_PRINT("%sMATCH%s: match: %s\n",
                                //                ANSI_COLOR_FG_BLUE, ANSI_COLOR_OFF, l_var_ref.m_data);
                                ao_var_ref_list->push_back(l_var_ref);

                        }
                }
                break;
        }
        default:
        {
                // Reflect Variable name
                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                waflz_pb::sec_rule_t_variable_t_type_t_descriptor()->FindValueByNumber(a_var.type());
                m_unimplemented_variables[l_descriptor->name()]++;
                break;
        }
        }


        return STATUS_OK;
}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl::run_op(const waflz_pb::sec_rule_t_operator_t &a_op,
                     const char *l_data,
                     match_list_t &ao_list)
{

        waflz_pb::sec_rule_t_operator_t_type_t l_op_type = waflz_pb::sec_rule_t_operator_t_type_t_RX;
        if(a_op.has_type())
        {
                l_op_type = a_op.type();
        }

        switch(l_op_type)
        {
        case waflz_pb::sec_rule_t_operator_t_type_t_RX:
        {
                // Apply regex match...
                NDBG_PRINT("%sREGEX%s: match: %s\n",
                                ANSI_COLOR_BG_YELLOW, ANSI_COLOR_OFF, l_data);

                if(!a_op.has_value() || !a_op.is_regex())
                {
                        break;
                }

                // Compile regex
                int32_t l_match_status;
                l_match_status = get_pcre_match_list(a_op.value().c_str(), l_data, ao_list);
                if(l_match_status != STATUS_OK)
                {
                        return STATUS_ERROR;
                }

                // Match...
                break;
        }
        default:
        {

                // Reflect Variable name
                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                waflz_pb::sec_rule_t_operator_t_type_t_descriptor()->FindValueByNumber(a_op.type());
                m_unimplemented_variables[l_descriptor->name()]++;
                break;

                break;
        }
        }

        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl::get_tx_data(const uint32_t a_tx,
                          const char *a_data,
                          char **ao_tx_data)
{

        NDBG_PRINT("%sTX%s: %u\n",
                        ANSI_COLOR_BG_BLUE, ANSI_COLOR_OFF, a_tx);

        // Apply transformation to string
        switch(a_tx)
        {
        case waflz_pb::sec_action_t_transformation_type_t_NONE:
        {
                // Do nothing...
                *ao_tx_data = const_cast <char *>(a_data);
                break;
        }
        default:
        {

                // Reflect Variable name
                const google::protobuf::EnumValueDescriptor* l_descriptor =
                                waflz_pb::sec_action_t_transformation_type_t_descriptor()->FindValueByNumber(a_tx);
                m_unimplemented_transformations[l_descriptor->name()]++;
                break;

        }
        }


        return STATUS_OK;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
static void show_rule_info(const waflz_pb::sec_rule_t &a_secrule)
{
        NDBG_OUTPUT("%s+---------------------------------------+ %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF);
        NDBG_OUTPUT("%s|                 RULE                  | %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF);
        NDBG_OUTPUT("%s+---------------------------------------+ %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF);
        const waflz_pb::sec_action_t &l_action = a_secrule.action();
        if(l_action.has_file())
                NDBG_OUTPUT("File:  %s\n",l_action.file().c_str());
        if(l_action.has_id())
                NDBG_OUTPUT("ID:    %s\n",l_action.id().c_str());
        if(a_secrule.has_order())
                NDBG_OUTPUT("Order: %d\n",a_secrule.order());

        std::string l_rule_short_str = a_secrule.ShortDebugString();
        colorize_string(l_rule_short_str);
        NDBG_OUTPUT("%s\n", l_rule_short_str.c_str());
        NDBG_OUTPUT("%s+---------------------------------------+ %s\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl::process_rule(const http_request &a_req,
                           const waflz_pb::sec_rule_t &a_secrule)
{

        show_rule_info(a_secrule);

        // Get action
        if(!a_secrule.has_action())
        {
                // TODO is OK???
                return STATUS_OK;
        }

        const waflz_pb::sec_action_t &l_action = a_secrule.action();

        if(!a_secrule.has_operator_())
        {
                // TODO this aight???
                return STATUS_OK;
        }

        // -------------------------------------------------
        // TODO !!!
        // -------------------------------------------------
        // Loop over variables in variable list
        // For each variable type...
        //   For each variable in that type list
        //      For each transformation
        //        Apply operator to variable
        // -------------------------------------------------
        // Variable loop
        // -------------------------------------------------
        for(int32_t i_var = 0; i_var < a_secrule.variable_size(); ++i_var)
        {
                int32_t l_status;

                // -------------------------------------------------
                // Extract list of variables
                // -------------------------------------------------
                const waflz_pb::sec_rule_t_variable_t& l_var = a_secrule.variable(i_var);
                var_ref_list_t l_var_ref_list;
                var_ref_list_t *l_var_ref_list_p = &l_var_ref_list;
                l_status = get_vars(l_var, l_var_ref_list_p, a_req);
                if(l_status != STATUS_OK)
                {
                        return STATUS_ERROR;
                }

                // TODO Check for count variables...
                //if(l_var.has_is_count() && l_var.is_count())
                //{
                //
                //}

                // TODO DEBUG???
                //if((i_var == 0) && (l_var_ref_list_p->size()))
                //{
                //        show_rule_info(a_secrule);
                //}

                // -------------------------------------------------
                // Variable loop
                // -------------------------------------------------
                for(var_ref_list_t::const_iterator i_v = l_var_ref_list_p->begin();
                    i_v != l_var_ref_list_p->end();
                    ++i_v)
                {
                        // -------------------------------------------------
                        // Transformation loop
                        // -------------------------------------------------

                        const char *l_data = i_v->m_data;

                        NDBG_PRINT("%sVAR%s: VAR: %s tx_size: %d\n",
                                        ANSI_COLOR_BG_MAGENTA, ANSI_COLOR_OFF, l_data, l_action.t_size());

                        // Set size to at least one if no tx specifiied
                        int32_t l_t_size = l_action.t_size()?l_action.t_size():1;
                        for(int32_t i_t = 0; i_t < l_t_size; ++i_t)
                        {

                                // In case there's no tx type
                                waflz_pb::sec_action_t_transformation_type_t l_t_type;
                                if(i_t > 1 || l_action.t_size())
                                {
                                        l_t_type = l_action.t(i_t);
                                }
                                else
                                {
                                        l_t_type = waflz_pb::sec_action_t_transformation_type_t_NONE;
                                }

                                char *l_tx_data;
                                l_status = get_tx_data(l_t_type, l_data, &l_tx_data);
                                if(l_status != STATUS_OK)
                                {
                                        return STATUS_ERROR;
                                }

                                if(a_secrule.has_operator_())
                                {
                                        match_list_t l_op_list;
                                        int32_t l_match_status;
                                        l_match_status = run_op(a_secrule.operator_(), l_tx_data, l_op_list);
                                        if(l_match_status != STATUS_OK)
                                        {
                                                return STATUS_ERROR;
                                        }
                                        if(!l_op_list.empty())
                                        {
                                                // Apply regex match...
                                                NDBG_PRINT("%sREGEX%s: MATCH: %s\n",
                                                                ANSI_COLOR_BG_RED, ANSI_COLOR_OFF, l_tx_data);

                                                m_last_matched_secrule = &a_secrule;
                                                return l_op_list.size();
                                        }
                                }
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
int32_t wafl::process_phase(const http_request &a_req,
                            const rule_list_t &a_rule_list)
{

        for(rule_list_t::const_iterator i_rule = a_rule_list.begin();
            i_rule != a_rule_list.end();
            ++i_rule)
        {
                int32_t l_status;
                l_status = process_rule(a_req, **i_rule);
                if(l_status == STATUS_ERROR)
                {
                        return STATUS_ERROR;
                }
                else if(l_status > 0)
                {
                        NDBG_PRINT("Gotta match.\n");
                        return l_status;
                }
        }

        return STATUS_OK;
}


//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t wafl::process_request(const http_request &a_req)
{

        if(!m_is_preprocessed)
        {
                preprocess();
        }

        int32_t l_status = STATUS_OK;
        int32_t l_match_status = STATUS_OK;

        // -------------------------------------------------
        // Phase: request headers
        // -------------------------------------------------
        l_match_status = process_phase(a_req, m_phase_request_headers);
        if(l_match_status == STATUS_ERROR)
        {
                return STATUS_ERROR;
        }
        else if(l_match_status > 0)
        {
                goto process_request_done;
        }


        // -------------------------------------------------
        // Phase: request body
        // -------------------------------------------------
        l_match_status = process_phase(a_req, m_phase_request_body);
        if(l_match_status == STATUS_ERROR)
        {
                return STATUS_ERROR;
        }
        else if(l_match_status > 0)
        {
                goto process_request_done;
        }


process_request_done:
        // -------------------------------------------------
        // Phase: logging
        // -------------------------------------------------
        l_status = process_phase(a_req, m_phase_logging);
        if(l_status == STATUS_ERROR)
        {
                return STATUS_ERROR;
        }

        return l_match_status;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl::wafl(void):
        m_is_initd(false),
        m_is_preprocessed(false),
        m_parser(),
        m_verbose(false),
        m_color(false),
        m_last_matched_secrule(NULL),
        m_config(NULL),
        m_phase_request_headers(),
        m_phase_request_body(),
        m_phase_response_headers(),
        m_phase_response_body(),
        m_phase_logging(),
        m_unimplemented_directives(),
        m_unimplemented_variables(),
        m_unimplemented_operators(),
        m_unimplemented_actions(),
        m_unimplemented_transformations()

{
        m_config = new waflz_pb::sec_config_t();
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
wafl::~wafl()
{
        if(m_config)
        {
                delete m_config;
                m_config = NULL;
        }
}


//: ----------------------------------------------------------------------------
//: Class variables
//: ----------------------------------------------------------------------------
