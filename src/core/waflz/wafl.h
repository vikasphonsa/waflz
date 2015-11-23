//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl.h
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
#ifndef _WAFL_H
#define _WAFL_H


//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "http_request.h"
#include "ndebug.h"
#include "wafl_parser.h"
#include <string>
#include <list>
#include <map>


//: ----------------------------------------------------------------------------
//: Constants
//: ----------------------------------------------------------------------------
#define MODSECURITY_RULE_PHASE_REQUEST_HEADERS       1
#define MODSECURITY_RULE_PHASE_REQUEST_BODY          2
#define MODSECURITY_RULE_PHASE_RESPONSE_HEADERS      3
#define MODSECURITY_RULE_PHASE_RESPONSE_BODY         4
#define MODSECURITY_RULE_PHASE_LOGGING               5

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Enums
//: ----------------------------------------------------------------------------


//: ----------------------------------------------------------------------------
//: Fwd Decl's
//: ----------------------------------------------------------------------------
namespace waflz_pb {
        class sec_rule_t;
        class sec_rule_t_variable_t;
        class sec_rule_t_operator_t;
        class sec_config_t;
};

class wafl_filter;

//: ----------------------------------------------------------------------------
//: Types
//: ----------------------------------------------------------------------------
typedef struct {
        const char *m_key;
        const char *m_data;
} var_ref_t;

typedef std::map <std::string, uint32_t> count_map_t;
typedef std::list <std::string> string_list_t;
typedef std::list <const waflz_pb::sec_rule_t *> rule_list_t;
typedef std::list <var_ref_t> var_ref_list_t;

//: ----------------------------------------------------------------------------
//: \details: TODO
//: ----------------------------------------------------------------------------
class wafl
{
public:

        // -------------------------------------------------
        // Public methods
        // -------------------------------------------------
        wafl();
        ~wafl();

        int32_t init(wafl_parser::format_t a_format, const std::string &a_path);

        // Settings...
        void set_verbose(bool a_val) { m_verbose = a_val;}
        void set_color(bool a_val) { m_color = a_val;}

        int32_t get_str(std::string &ao_str, wafl_parser::format_t a_format);
        int32_t get_last_matched_rule_str(std::string &ao_str);
        void show_status(void);
        void show(void);
        void show_debug(void);

        // -------------------------------------------------
        // process
        // -------------------------------------------------
        int32_t process_request(const http_request &a_req);

        // -------------------------------------------------
        // Filter
        // -------------------------------------------------
        int32_t filter(wafl_filter &a_filter);

        // -------------------------------------------------
        // Class methods
        // -------------------------------------------------

        // -------------------------------------------------
        // Public members
        // -------------------------------------------------

private:
        // -------------------------------------------------
        // Private methods
        // -------------------------------------------------
        DISALLOW_COPY_AND_ASSIGN(wafl)

        // -------------------------------------------------
        // process
        // -------------------------------------------------
        void preprocess(void);
        int32_t process_phase(const http_request &a_req,
                              const rule_list_t &a_rule_list);
        int32_t process_rule(const http_request &a_req,
                             const waflz_pb::sec_rule_t &a_secrule);

        int32_t get_vars(const waflz_pb::sec_rule_t_variable_t &a_var,
                         var_ref_list_t *ao_var_ref_list,
                         const http_request &a_req);

        int32_t get_tx_data(const uint32_t a_tx,
                            const char *a_data,
                            char **ao_tx_data);

        int32_t run_op(const waflz_pb::sec_rule_t_operator_t &a_op,
                       const char *l_data,
                       match_list_t &ao_list);



        // -------------------------------------------------
        // Private members
        // -------------------------------------------------
        bool m_is_initd;
        bool m_is_preprocessed;

        wafl_parser m_parser;

        // -------------------------------------------------
        // Settings
        // -------------------------------------------------
        bool m_verbose;
        bool m_color;

        // -------------------------------------------------
        // Stats
        // -------------------------------------------------
        const waflz_pb::sec_rule_t *m_last_matched_secrule;

        // -------------------------------------------------
        // Protobuf
        // -------------------------------------------------
        waflz_pb::sec_config_t *m_config;

        // Preprocessed...
        rule_list_t m_phase_request_headers;
        rule_list_t m_phase_request_body;
        rule_list_t m_phase_response_headers;
        rule_list_t m_phase_response_body;
        rule_list_t m_phase_logging;

        // Debugging
        count_map_t m_unimplemented_directives;
        count_map_t m_unimplemented_variables;
        count_map_t m_unimplemented_operators;
        count_map_t m_unimplemented_actions;
        count_map_t m_unimplemented_transformations;


        // -------------------------------------------------
        // Class members
        // -------------------------------------------------

};


#endif





