//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl_parser.h
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
#ifndef _WAFL_PARSER_H
#define _WAFL_PARSER_H


//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "ndebug.h"
#include <string>
#include <list>
#include <map>


//: ----------------------------------------------------------------------------
//: Constants
//: ----------------------------------------------------------------------------
#define MODSECURITY_RULE_INDENT_SIZE 4

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
        class sec_action_t;
        class sec_rule_t;
        class sec_config_t;
};

//: ----------------------------------------------------------------------------
//: Types
//: ----------------------------------------------------------------------------
typedef std::map <std::string, uint32_t> count_map_t;
typedef std::list <std::string> string_list_t;

typedef struct _kv {
	std::string m_key;
	string_list_t m_list;

	_kv():
		m_key(),
		m_list()
	{};

} kv_t;

typedef std::list <kv_t> kv_list_t;

typedef std::list <const waflz_pb::sec_rule_t *> rule_list_t;
typedef std::list <std::string> match_list_t;

//: ----------------------------------------------------------------------------
//: \details: TODO
//: ----------------------------------------------------------------------------
class wafl_parser
{
public:

        typedef enum {

                PROTOBUF = 0,
                JSON,
                MODSECURITY,
                DESC

        } format_t;

        // -------------------------------------------------
        // Public methods
        // -------------------------------------------------
        wafl_parser();
        ~wafl_parser();


        // Settings...
        void set_verbose(bool a_val) { m_verbose = a_val;}
        void set_color(bool a_val) { m_color = a_val;}

        int32_t parse_config(format_t a_format, const std::string &a_path, waflz_pb::sec_config_t *a_config);
        int32_t get_modsec_config_str(waflz_pb::sec_config_t *a_config, std::string &ao_str);
        void show_status(void);

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
        DISALLOW_COPY_AND_ASSIGN(wafl_parser)

        // -------------------------------------------------
        // configs
        // -------------------------------------------------
        // Generate read entry points
        int32_t read_directory(format_t a_format, const std::string &a_directory);
        int32_t read_file(format_t a_format, const std::string &a_file, bool a_force = false);

        // -------------------------------------------------
        // pbuf helpers
        // -------------------------------------------------
        int32_t read_file_pbuf(const std::string &a_file, bool a_force = false);

        // -------------------------------------------------
        // Json helpers
        // -------------------------------------------------
        int32_t read_file_json(const std::string &a_file, bool a_force = false);

        // -------------------------------------------------
        // Modsecurity Rule helpers
        // -------------------------------------------------
        int32_t read_file_modsec(const std::string &a_file,
                                 bool a_force = false);
        int32_t read_line(const char *a_line, std::string &ao_cur_line,
                          uint32_t a_line_len);
        int32_t read_wholeline(const char *a_line,
                               uint32_t a_line_len);
        int32_t read_secrule(const char *a_line,
                             uint32_t a_line_len);
        int32_t read_secaction(const char *a_line,
                             uint32_t a_line_len,
                             bool a_is_default);
        int32_t get_next_string(char **ao_line, uint32_t *ao_char, uint32_t a_line_len, std::string &ao_string);
        int32_t get_strings_from_line(const char *a_line, uint32_t a_line_len, string_list_t &ao_str_list);
        int32_t tokenize_kv_list(const std::string &a_string, const char a_delimiter, kv_list_t &ao_kv_list);
        int32_t get_modsec_rule_line(std::string &ao_str,
                                     const waflz_pb::sec_rule_t &a_secrule,
                                     const uint32_t a_indent,
                                     const bool a_is_chained);
        int32_t append_modsec_rule(std::string &ao_str,
                                   const waflz_pb::sec_rule_t &a_secrule,
                                   const uint32_t a_indent,
                                   const bool a_is_chained);


        // Add rules...
        int32_t add_rule(const kv_list_t &a_variable_list,
                         const std::string &a_operator_fx,
                         const std::string &a_operator_match,
                         const kv_list_t & a_action_list);

        // Add Action...
        int32_t add_action(waflz_pb::sec_action_t *ao_action,
                           const kv_list_t & a_action_list,
                           bool &ao_is_chained);

        // -------------------------------------------------
        // Errors
        // -------------------------------------------------
        int32_t show_config_error(const char *a_file, const char *a_func, uint32_t a_line, const char *a_msg);

        // -------------------------------------------------
        // Private members
        // -------------------------------------------------

        // -------------------------------------------------
        // Settings
        // -------------------------------------------------
        bool m_verbose;
        bool m_color;

        // -------------------------------------------------
        // Stats
        // -------------------------------------------------
        std::string m_cur_file;
        std::string m_cur_file_base;
        std::string m_cur_line;
        uint32_t m_cur_line_num;
        uint32_t m_cur_line_pos;
        waflz_pb::sec_rule_t *m_cur_parent_rule;

        // -------------------------------------------------
        // Protobuf
        // -------------------------------------------------
        waflz_pb::sec_config_t *m_config;


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

//: ----------------------------------------------------------------------------
//: Prototypes
//: ----------------------------------------------------------------------------
int32_t get_pcre_match_list(const char *a_regex, const char *a_str, match_list_t &ao_match_list);


#endif








