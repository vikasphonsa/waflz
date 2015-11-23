//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    http_request.cc
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

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "http_request.h"
#include "util.h"
#include "ndebug.h"

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void http_request::set_uri(const std::string &a_uri)
{
        m_uri = a_uri;

        // -------------------------------------------------
        // TODO Zero copy with something like substring...
        // This is pretty awful performance wise
        // -------------------------------------------------
        // Read uri up to first '?'
        size_t l_query_pos = 0;
        if((l_query_pos = m_uri.find('?', 0)) == std::string::npos)
        {
                // No query string -path == uri
                m_path = m_uri;
                return;
        }

        m_path = m_uri.substr(0, l_query_pos);

        // TODO Url decode???

        std::string l_query = m_uri.substr(l_query_pos + 1, m_uri.length() - l_query_pos + 1);

        // Split the query by '&'
        if(!l_query.empty())
        {

                //NDBG_PRINT("%s__QUERY__%s: l_query: %s\n", ANSI_COLOR_BG_WHITE, ANSI_COLOR_OFF, l_query.c_str());

                size_t l_qi_begin = 0;
                size_t l_qi_end = 0;
                bool l_last = false;
                while (!l_last)
                {
                        l_qi_end = l_query.find('&', l_qi_begin);
                        if(l_qi_end == std::string::npos)
                        {
                                l_last = true;
                                l_qi_end = l_query.length();
                        }

                        std::string l_query_item = l_query.substr(l_qi_begin, l_qi_end - l_qi_begin);

                        // Search for '='
                        size_t l_qi_val_pos = 0;
                        l_qi_val_pos = l_query_item.find('=', 0);
                        std::string l_q_k;
                        std::string l_q_v;
                        if(l_qi_val_pos != std::string::npos)
                        {
                                l_q_k = l_query_item.substr(0, l_qi_val_pos);
                                l_q_v = l_query_item.substr(l_qi_val_pos + 1, l_query_item.length());
                        }
                        else
                        {
                                l_q_k = l_query_item;
                        }

                        //NDBG_PRINT("%s__QUERY__%s: k[%s]: %s\n",
                        //                ANSI_COLOR_BG_WHITE, ANSI_COLOR_OFF, l_q_k.c_str(), l_q_v.c_str());

                        // Add to list
                        kv_list_map_t::iterator i_key = m_query.find(l_q_k);
                        if(i_key == m_query.end())
                        {
                                value_list_t l_list;
                                l_list.push_back(l_q_v);
                                m_query[l_q_k] = l_list;
                        }
                        else
                        {
                                i_key->second.push_back(l_q_v);
                        }

                        // Move fwd
                        l_qi_begin = l_qi_end + 1;

                }
        }

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void http_request::set_header(const std::string &a_key, const std::string &a_val)
{
        kv_list_map_t::iterator i_header = m_headers.find(a_key);
        if(i_header == m_headers.end())
        {
                value_list_t l_list;
                l_list.push_back(a_val);
                m_headers[a_key] = l_list;
        }
        else
        {
                i_header->second.push_back(a_val);
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void http_request::set_body(const std::string &a_body)
{
        m_body = a_body;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const kv_list_map_t &http_request::get_uri_decoded_query(void)
{

        if(m_query_uri_decoded.empty() && !m_query.empty())
        {

                // Decode the arguments for now
                for(kv_list_map_t::const_iterator i_kv = m_query.begin();
                    i_kv != m_query.end();
                    ++i_kv)
                {
                        value_list_t l_value_list;
                        for(value_list_t::const_iterator i_v = i_kv->second.begin();
                            i_v != i_kv->second.end();
                            ++i_v)
                        {
                                std::string l_v = uri_decode(*i_v);
                                l_value_list.push_back(l_v);
                        }

                        std::string l_k = uri_decode(i_kv->first);
                        m_query_uri_decoded[l_k] = l_value_list;
                }
        }

        return m_query_uri_decoded;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void http_request::clear()
{
        m_uri.clear();
        m_path.clear();
        m_query.clear();
        m_headers.clear();
        m_body.clear();

        // Uri decoded
        m_query_uri_decoded.clear();

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void http_request::show(bool a_color)
{
        std::string l_host_color = "";
        std::string l_query_color = "";
        std::string l_header_color = "";
        std::string l_body_color = "";
        std::string l_off_color = "";
        if(a_color)
        {
                l_host_color = ANSI_COLOR_FG_BLUE;
                l_query_color = ANSI_COLOR_FG_MAGENTA;
                l_header_color = ANSI_COLOR_FG_GREEN;
                l_body_color = ANSI_COLOR_FG_YELLOW;
                l_off_color = ANSI_COLOR_OFF;
        }

        // Host
        NDBG_OUTPUT("%sUri%s:  %s\n", l_host_color.c_str(), l_off_color.c_str(), m_uri.c_str());
        NDBG_OUTPUT("%sPath%s: %s\n", l_host_color.c_str(), l_off_color.c_str(), m_path.c_str());

        // Query
        for(kv_list_map_t::iterator i_key = m_query.begin();
                        i_key != m_query.end();
            ++i_key)
        {
                NDBG_OUTPUT("%s%s%s: %s\n",
                                l_query_color.c_str(), i_key->first.c_str(), l_off_color.c_str(),
                                i_key->second.begin()->c_str());
        }


        // Headers
        for(kv_list_map_t::iterator i_key = m_headers.begin();
            i_key != m_headers.end();
            ++i_key)
        {
                NDBG_OUTPUT("%s%s%s: %s\n",
                                l_header_color.c_str(), i_key->first.c_str(), l_off_color.c_str(),
                                i_key->second.begin()->c_str());
        }

        // Body
        NDBG_OUTPUT("%sBody%s: %s\n", l_body_color.c_str(), l_off_color.c_str(), m_body.c_str());


}

