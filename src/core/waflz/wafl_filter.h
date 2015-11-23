//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    wafl_filter.h
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
#ifndef _WAFL_FILTER_H
#define _WAFL_FILTER_H


//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "ndebug.h"

#include <string>
#include <set>
#include <list>
#include <jsoncpp/json/json.h>


//: ----------------------------------------------------------------------------
//: Constants
//: ----------------------------------------------------------------------------


//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Enums
//: ----------------------------------------------------------------------------


//: ----------------------------------------------------------------------------
//: Types
//: ----------------------------------------------------------------------------


//: ----------------------------------------------------------------------------
//: Fwd Decl's
//: ----------------------------------------------------------------------------
namespace waflz_pb {
        class sec_config_t;
        class sec_rule_t;
};


//: ----------------------------------------------------------------------------
//: \details: TODO
//: ----------------------------------------------------------------------------
class wafl_filter
{
public:

        typedef std::set<std::string> rule_var_set_t;
        typedef enum {
                MSG = 0,
                FILE
        } filter_t;
        typedef struct match {
                filter_t m_type;
                std::string m_match;
                bool m_is_negated;

                match():
                        m_type(MSG),
                        m_match(),
                        m_is_negated(false)
                {}


        } match_t;
        typedef std::list<match_t> match_list_t;

        bool match(const waflz_pb::sec_rule_t &a_rule);

        // -------------------------------------------------
        // Public methods
        // -------------------------------------------------
        wafl_filter();
        ~wafl_filter();

        int32_t init_with_file(const std::string &a_file);
        int32_t init_with_line(const std::string &a_line);

        // Settings...

        // -------------------------------------------------
        // Class methods
        // -------------------------------------------------

        // -------------------------------------------------
        // Public members
        // -------------------------------------------------
        Json::Value *m_filter_json;
private:
        // -------------------------------------------------
        // Private methods
        // -------------------------------------------------
        DISALLOW_COPY_AND_ASSIGN(wafl_filter)

        // -------------------------------------------------
        // Private members
        // -------------------------------------------------
        bool m_is_initd;

        rule_var_set_t m_var_set;
        match_list_t m_match_list;

        void add_match(filter_t a_filter, std::string &a_match, bool a_is_negated);
        std::string get_tx_setvar(const std::string &a_var);
        bool has_setvar(const waflz_pb::sec_rule_t &a_rule);
        void push_setvar(const waflz_pb::sec_rule_t &a_rule);

        bool match_msg(const waflz_pb::sec_rule_t &a_rule, const std::string &a_match);
        bool match_file(const waflz_pb::sec_rule_t &a_rule, const std::string &a_match);

        // -------------------------------------------------
        // Settings
        // -------------------------------------------------


        // -------------------------------------------------
        // Class members
        // -------------------------------------------------

};


#endif







