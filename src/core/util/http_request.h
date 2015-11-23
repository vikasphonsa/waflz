//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    http_request.h
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    12/07/2014
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
#ifndef _HTTP_REQUEST_H
#define _HTTP_REQUEST_H

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "ndebug.h"
#include <string>
#include <map>
#include <list>

//: ----------------------------------------------------------------------------
//: Fwd' Decls
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Enums
//: ----------------------------------------------------------------------------


//: ----------------------------------------------------------------------------
//: Types
//: ----------------------------------------------------------------------------
typedef std::list <std::string> value_list_t;
typedef std::map<std::string, value_list_t> kv_list_map_t;

//: ----------------------------------------------------------------------------
//: \details: reqlet
//: ----------------------------------------------------------------------------
class http_request
{
public:
        // -------------------------------------------
        // Public methods
        // -------------------------------------------
        http_request(void):
                m_uri(),
                m_path(),
                m_query(),
                m_headers(),
                m_body(),

                // uri decoded...
                m_query_uri_decoded()
        {};
        ~http_request() {};

        void set_uri(const std::string &a_uri);
        void set_header(const std::string &a_key, const std::string &a_val);
        void set_body(const std::string &a_body);
        void show(bool a_color);
        void clear();

        // -------------------------------------------
        // Public members
        // -------------------------------------------
        // TODO make private and use getters...
        std::string m_uri;
        std::string m_path;
        kv_list_map_t m_query;
        kv_list_map_t m_headers;
        std::string m_body;

        // TODO Cookies!!!

        const kv_list_map_t &get_uri_decoded_query(void);

        // -------------------------------------------
        // Class methods
        // -------------------------------------------

private:
        // -------------------------------------------
        // Private methods
        // -------------------------------------------
        DISALLOW_COPY_AND_ASSIGN(http_request)


        // -------------------------------------------
        // Private members
        // -------------------------------------------

        // uri decoded version
        kv_list_map_t m_query_uri_decoded;


        // -------------------------------------------
        // Class members
        // -------------------------------------------

};

//: ----------------------------------------------------------------------------
//: Prototypes
//: ----------------------------------------------------------------------------


#endif //#ifndef _HTTP_REQUEST_H
