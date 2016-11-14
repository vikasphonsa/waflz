//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    jspb.cc
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
#include "jspb.h"

#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------
#define JSPB_ERR_LEN 4096
#define JSPB_PERROR(...) snprintf(g_err_msg, JSPB_ERR_LEN, __VA_ARGS__)

//: ----------------------------------------------------------------------------
//: Globals
//: ----------------------------------------------------------------------------
static char g_err_msg[JSPB_ERR_LEN];

namespace ns_jspb {

//: ----------------------------------------------------------------------------
//: \details: Set a single field value in the json object.
//: \return:  TODO
//: \param:   reflection the protobuf message reflection informationm
//: \param:   field the protobuf message field information
//: \param:   message the protobuf message to read from
//: \param:   value the json object to update
//: ----------------------------------------------------------------------------
static int32_t convert_single_field(rapidjson::Value &ao_val,
                                    rapidjson::Document::AllocatorType& a_alx,
                                    const google::protobuf::Reflection* a_ref,
                                    const google::protobuf::FieldDescriptor* a_field,
                                    const google::protobuf::Message& a_msg)
{
        switch (a_field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetDouble(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetFloat(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetInt64(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetUInt64(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetInt32(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetUInt32(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 a_ref->GetBool(a_msg, a_field),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 rapidjson::Value(a_ref->GetString(a_msg, a_field).c_str(), a_alx).Move(),
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
        {
                JSPB_PERROR("group type not supported for field: '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        {
                rapidjson::Value l_obj;
                l_obj.SetObject();
                int32_t l_s;
                l_s = convert_to_json(l_obj, a_alx, a_ref->GetMessage(a_msg, a_field));
                if(l_s != JSPB_OK)
                {
                        return JSPB_ERROR;
                }
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 l_obj,
                                 a_alx);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
        {
                JSPB_PERROR("binary type not supported for field: '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
                ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(),
                                 rapidjson::Value(a_ref->GetEnum(a_msg, a_field)->name().c_str(), a_alx).Move(),
                                 a_alx);
                break;
        }
        default:
        {
                JSPB_PERROR("unknown type in field: '%s' type: %d", a_field->full_name().c_str(), a_field->type());
                return JSPB_ERROR;
        }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a repeated field to json for various types.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   converter the member function to do the conversion
//: \param:   field the field descriptor for the field we are converting
//: \param:   message the message being converted
//: \param:   value the value to store the converted data
//: ----------------------------------------------------------------------------
template <typename T, typename F1>
static int32_t convert_repeated_field(rapidjson::Value &ao_val,
                                      rapidjson::Document::AllocatorType& a_alx,
                                      const google::protobuf::Reflection* a_ref,
                                      F1 converter,
                                      const google::protobuf::FieldDescriptor* a_field,
                                      const google::protobuf::Message& a_msg)
{
        // Add array object...
        rapidjson::Value l_arr(rapidjson::kArrayType);
        for (int i_f = 0; i_f < a_ref->FieldSize(a_msg, a_field); ++i_f)
        {
                l_arr.PushBack(rapidjson::Value((T)(a_ref->*converter)(a_msg, a_field, i_f)).Move(),
                               a_alx);
        }
        ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(), l_arr, a_alx);
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a repeated field to json for various types.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   converter the member function to do the conversion
//: \param:   field the field descriptor for the field we are converting
//: \param:   message the message being converted
//: \param:   value the value to store the converted data
//: ----------------------------------------------------------------------------
template <typename F1>
static int32_t convert_repeated_field_str(rapidjson::Value &ao_val,
                                          rapidjson::Document::AllocatorType& a_alx,
                                          const google::protobuf::Reflection* a_ref,
                                          F1 converter,
                                          const google::protobuf::FieldDescriptor* a_field,
                                          const google::protobuf::Message& a_msg)
{
        // Add array object...
        rapidjson::Value l_arr(rapidjson::kArrayType);
        for (int i_f = 0; i_f < a_ref->FieldSize(a_msg, a_field); ++i_f)
        {
                l_arr.PushBack(rapidjson::Value(((a_ref->*converter)(a_msg, a_field, i_f)).c_str(), a_alx).Move(),
                a_alx);
        }
        ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(), l_arr, a_alx);
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a repeated field to json for a message type.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are converting
//: \param:   message the message being converted
//: \param:   value the value to store the converted data
//: ----------------------------------------------------------------------------
static int32_t convert_repeated_message_field(rapidjson::Value &ao_val,
                                              rapidjson::Document::AllocatorType& a_alx,
                                              const google::protobuf::Reflection* a_ref,
                                              const google::protobuf::FieldDescriptor* a_field,
                                              const google::protobuf::Message& a_msg)
{
        // Add array object...
        rapidjson::Value l_arr(rapidjson::kArrayType);
        for (int i_f = 0; i_f < a_ref->FieldSize(a_msg, a_field); ++i_f)
        {
                int32_t l_s;
                rapidjson::Value l_val;
                l_s = convert_to_json(l_val, a_alx, a_ref->GetRepeatedMessage(a_msg, a_field, i_f));
                if(l_s != JSPB_OK)
                {
                        return JSPB_ERROR;
                }
                l_arr.PushBack(l_val, a_alx);
        }
        ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(), l_arr, a_alx);
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a repeated field to json for an enum type.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are converting
//: \param:   message the message being converted
//: \param:   value the value to store the converted data
//: ----------------------------------------------------------------------------
static int32_t convert_repeated_enum_field(rapidjson::Value &ao_val,
                                           rapidjson::Document::AllocatorType& a_alx,
                                           const google::protobuf::Reflection* a_ref,
                                           const google::protobuf::FieldDescriptor* a_field,
                                           const google::protobuf::Message& a_msg)
{
        // Add array object...
        rapidjson::Value l_arr(rapidjson::kArrayType);
        for (int i_f = 0; i_f < a_ref->FieldSize(a_msg, a_field); ++i_f)
        {
                l_arr.PushBack(rapidjson::Value(a_ref->GetRepeatedEnum(a_msg, a_field, i_f)->name().c_str(), a_alx).Move(),
                               a_alx);
                // TODO Error checking...
        }
        ao_val.AddMember(rapidjson::Value(a_field->name().c_str(), a_alx).Move(), l_arr, a_alx);
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a repeated field to json.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are converting
//: \param:   message the message being converted
//: \param:   value the value to store the converted data
//: ----------------------------------------------------------------------------
static int32_t convert_repeated_field(rapidjson::Value &ao_val,
                                      rapidjson::Document::AllocatorType& a_alx,
                                      const google::protobuf::Reflection* a_ref,
                                      const google::protobuf::FieldDescriptor* a_field,
                                      const google::protobuf::Message& a_msg)
{
        int32_t l_s = JSPB_OK;
        switch (a_field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        {
                l_s = convert_repeated_field<double>(ao_val,
                                                     a_alx,
                                                     a_ref,
                                                     &google::protobuf::Reflection::GetRepeatedUInt64,
                                                     a_field,
                                                     a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        {
                l_s = convert_repeated_field<double>(ao_val,
                                                     a_alx,
                                                     a_ref,
                                                     &google::protobuf::Reflection::GetRepeatedFloat,
                                                     a_field,
                                                     a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        {
                l_s = convert_repeated_field<int64_t>(ao_val,
                                                      a_alx,
                                                      a_ref,
                                                      &google::protobuf::Reflection::GetRepeatedInt64,
                                                      a_field,
                                                      a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        {
                l_s = convert_repeated_field<uint64_t>(ao_val,
                                                       a_alx,
                                                       a_ref,
                                                       &google::protobuf::Reflection::GetRepeatedUInt64,
                                                       a_field,
                                                       a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        {
                l_s = convert_repeated_field<int32_t>(ao_val,
                                                      a_alx,
                                                      a_ref,
                                                      &google::protobuf::Reflection::GetRepeatedInt32,
                                                      a_field,
                                                      a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        {
                l_s = convert_repeated_field<uint32_t>(ao_val,
                                                       a_alx,
                                                       a_ref,
                                                       &google::protobuf::Reflection::GetRepeatedUInt32,
                                                       a_field,
                                                       a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
        {
                l_s = convert_repeated_field<bool>(ao_val,
                                                   a_alx,
                                                   a_ref,
                                                   &google::protobuf::Reflection::GetRepeatedBool,
                                                   a_field,
                                                   a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING:
        {
                l_s = convert_repeated_field_str(ao_val,
                                                 a_alx,
                                                 a_ref,
                                                 &google::protobuf::Reflection::GetRepeatedString,
                                                 a_field,
                                                 a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
        {
                JSPB_PERROR("group type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        {
                convert_repeated_message_field(ao_val,
                                               a_alx,
                                               a_ref,
                                               a_field,
                                               a_msg);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
        {
                JSPB_PERROR("binary type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
                l_s = convert_repeated_enum_field(ao_val,
                                                  a_alx,
                                                  a_ref,
                                                  a_field,
                                                  a_msg);
                break;
        }
        default:
        {
                JSPB_PERROR("unknown type in field: '%s': %d", a_field->full_name().c_str(), a_field->type());
                return JSPB_ERROR;
        }
        }
        return l_s;
}

//: ----------------------------------------------------------------------------
//: \details: Update a field within a message from a given json object,
//:           for many types of values.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   updater the method to use to update the field
//: \param:   value the value to get the information from
//: \param:   access the method used to get the value from the json object
//: \param:   checker the method used to check that the json type is correct
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
template <typename F1, typename F2, typename F3>
static int32_t update_field(google::protobuf::Message& ao_msg,
                            const google::protobuf::Reflection* a_ref,
                            const google::protobuf::FieldDescriptor* a_field,
                            F1 a_updater,
                            const rapidjson::Value &a_val,
                            F2 a_accessor,
                            F3 a_checker)
{
        // ---------------------------------------------------------------------
        // TODO FIX!!!
        // ---------------------------------------------------------------------
        if (!((a_val.*a_checker)()))
        {
                JSPB_PERROR("expecting type: %s for field: '%s'", a_field->name().c_str(), a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        (a_ref->*a_updater)(&ao_msg, a_field, (a_val.*a_accessor)());
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Update single enum field within message from json object.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   value the value to get the information from
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
template <typename F1>
static int32_t update_enum_field(google::protobuf::Message& ao_msg,
                                 const google::protobuf::Reflection* a_ref,
                                 const google::protobuf::Descriptor* a_des,
                                 const google::protobuf::FieldDescriptor* a_field,
                                 F1 a_updater,
                                 const rapidjson::Value &a_val)
{
        // ---------------------------------------------------------------------
        // TODO FIX!!!
        // ---------------------------------------------------------------------
        if (!a_val.IsString())
        {
                JSPB_PERROR("expecting string (enum) for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }

        const google::protobuf::EnumValueDescriptor* enumValueDescriptor = a_des->FindEnumValueByName(a_val.GetString());
        if (0 == enumValueDescriptor)
        {
                JSPB_PERROR("unknown enum for field ' '%s': %s", a_field->full_name().c_str(), a_val.GetString());
                return JSPB_ERROR;
        }
        (a_ref->*a_updater)(&ao_msg, a_field, enumValueDescriptor);
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Update single field within message from json object.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   value the value to get the information from
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
static int32_t update_single_field(google::protobuf::Message& ao_msg,
                                   const google::protobuf::Reflection* a_ref,
                                   const google::protobuf::Descriptor* a_des,
                                   const google::protobuf::FieldDescriptor* a_field,
                                   const rapidjson::Value &a_val)
{
        int32_t l_s = JSPB_OK;
        switch (a_field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        {
                l_s = update_field(ao_msg,
                                   a_ref,
                                   a_field,
                                   &google::protobuf::Reflection::SetDouble,
                                   a_val,
                                   &rapidjson::GenericValue<rapidjson::UTF8<> >::GetDouble,
                                   &rapidjson::GenericValue<rapidjson::UTF8<> >::IsNumber);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetFloat,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetFloat,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsNumber);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetInt64,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetInt64,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsInt64);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetUInt64,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetUint64,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsUint64);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetInt32,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetInt,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsInt);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetUInt32,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetUint,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsUint);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetBool,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetBool,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsBool);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING:
        {
                update_field(ao_msg,
                             a_ref,
                             a_field,
                             &google::protobuf::Reflection::SetString,
                             a_val,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::GetString,
                             &rapidjson::GenericValue<rapidjson::UTF8<> >::IsString);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
        {
                JSPB_PERROR("group type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        {
                l_s = update_from_json(*(a_ref->MutableMessage(&ao_msg, a_field)),
                                       a_val);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
        {
                JSPB_PERROR("binary type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
                l_s = update_enum_field(ao_msg,
                                        a_ref,
                                        a_des,
                                        a_field,
                                        &google::protobuf::Reflection::SetEnum,
                                        a_val);
                break;
        }
        default:
        {
                JSPB_PERROR("unknown type in field: '%s': %d", a_field->full_name().c_str(), a_field->type());
                return JSPB_ERROR;
        }
        }
        return l_s;
}

//: ----------------------------------------------------------------------------
//: \details: Update a repeated field within a message from a given json object,
//:           for many types of values.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   updater the method to use to update the field
//: \param:   value the value to get the information from
//: \param:   access the method used to get the value from the json object
//: \param:   checker the method used to check that the json type is correct
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
template <typename F1, typename F2, typename F3>
static int32_t update_repeated_field(google::protobuf::Message& ao_msg,
                                     const google::protobuf::Reflection* a_ref,
                                     const google::protobuf::FieldDescriptor* a_field,
                                     F1 a_updater,
                                     const rapidjson::Value &a_val,
                                     F2 a_accessor,
                                     F3 a_checker)
{
        for(rapidjson::Value::ConstValueIterator i_m = a_val.Begin();
            i_m != a_val.End();
            ++i_m)
        {
                int32_t l_s;
                l_s = update_field(ao_msg,
                                   a_ref,
                                   a_field,
                                   a_updater,
                                   *i_m,
                                   a_accessor,
                                   a_checker);
                if(l_s != JSPB_OK)
                {
                        return JSPB_ERROR;
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Update repeated message field within message from json object.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   value the value to get the information from
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
static int32_t update_repeated_message_field(google::protobuf::Message& ao_msg,
                                             const google::protobuf::Reflection* a_ref,
                                             const google::protobuf::FieldDescriptor* a_field,
                                             const rapidjson::Value &a_val)
{
        for(rapidjson::Value::ConstValueIterator i_m = a_val.Begin();
            i_m != a_val.End();
            ++i_m)
        {
                google::protobuf::Message* l_child = a_ref->AddMessage(&ao_msg, a_field);
                int32_t l_s;
                l_s = update_from_json(*l_child, *i_m);
                if(l_s != JSPB_OK)
                {
                        return JSPB_ERROR;
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Update repeated enum field within message from json object.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   value the value to get the information from
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
static int32_t update_repeated_enum_field(google::protobuf::Message& ao_msg,
                                          const google::protobuf::Reflection* a_ref,
                                          const google::protobuf::Descriptor* a_des,
                                          const google::protobuf::FieldDescriptor* a_field,
                                          const rapidjson::Value &a_val)
{
        for(rapidjson::Value::ConstValueIterator i_m = a_val.Begin();
            i_m != a_val.End();
            ++i_m)
        {
                int32_t l_s;
                l_s = update_enum_field(ao_msg,
                                        a_ref,
                                        a_des,
                                        a_field,
                                        &google::protobuf::Reflection::AddEnum,
                                        *i_m);
                if(l_s != JSPB_OK)
                {
                        return JSPB_ERROR;
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Update repeated field within message from json object.
//: \return:  TODO
//: \param:   reflection the reflection interface for the message
//: \param:   field the field descriptor for the field we are updating
//: \param:   value the value to get the information from
//: \param:   message the message being updated
//: ----------------------------------------------------------------------------
static int32_t update_repeated_field(google::protobuf::Message& ao_msg,
                                     const google::protobuf::Reflection* a_ref,
                                     const google::protobuf::Descriptor* a_des,
                                     const google::protobuf::FieldDescriptor* a_field,
                                     const rapidjson::Value &a_val)
{
        int32_t l_s;
        switch (a_field->type())
        {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddDouble,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetDouble,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsNumber);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddFloat,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetFloat,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsNumber);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddInt64,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetInt64,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsInt64);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddUInt64,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetUint64,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsUint64);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddInt32,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetInt,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsInt);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddUInt32,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetUint,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsUint);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddBool,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetBool,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsBool);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_STRING:
        {
                l_s = update_repeated_field(ao_msg,
                                            a_ref,
                                            a_field,
                                            &google::protobuf::Reflection::AddString,
                                            a_val,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::GetString,
                                            &rapidjson::GenericValue<rapidjson::UTF8<> >::IsString);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
        {
                JSPB_PERROR("group type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        {
                l_s = update_repeated_message_field(ao_msg,
                                                    a_ref,
                                                    a_field,
                                                    a_val);
                break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
        {
                JSPB_PERROR("binary type not supported for field '%s'", a_field->full_name().c_str());
                return JSPB_ERROR;
        }
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
        {
                l_s = update_repeated_enum_field(ao_msg,
                                                 a_ref,
                                                 a_des,
                                                 a_field,
                                                 a_val);
                break;
        }
        default:
        {
                JSPB_PERROR("unknown type in field: '%s': %d", a_field->full_name().c_str(), a_field->type());
                return JSPB_ERROR;
        }
        }
        return l_s;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t convert_to_json(rapidjson::Value &ao_val,
                        rapidjson::Document::AllocatorType &a_alx,
                        const google::protobuf::Message& a_msg)
{
        rapidjson::Value &l_v = ao_val.SetObject();
        const google::protobuf::Reflection* l_ref = a_msg.GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> l_fs;
        l_ref->ListFields(a_msg, &l_fs);
        for (std::vector<const google::protobuf::FieldDescriptor*>::const_iterator i_f = l_fs.begin();
             i_f != l_fs.end();
             ++i_f)
        {
                const google::protobuf::FieldDescriptor* l_f = *i_f;

                switch (l_f->label())
                {
                case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
                case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = convert_single_field(l_v, a_alx, l_ref, l_f, a_msg);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                case google::protobuf::FieldDescriptor::LABEL_REPEATED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = convert_repeated_field(l_v, a_alx, l_ref, l_f, a_msg);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                default:
                {
                        JSPB_PERROR("unknown label in field '%s': %d", l_f->full_name().c_str(), l_f->label());
                        return JSPB_ERROR;
                }
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a protobuf message to a json object, storing
//:           the result in a rapidjson::Document object.
//: \return:  TODO
//: \param:   msg the protobuf message to convert
//: \param:   value json object to hold the converted value
//: ----------------------------------------------------------------------------
int32_t convert_to_json(rapidjson::Document& ao_js,
                        const google::protobuf::Message& a_msg)
{
        rapidjson::Document::AllocatorType& l_alx = ao_js.GetAllocator();
        rapidjson::Value &l_v = ao_js.SetObject();
        const google::protobuf::Reflection* l_ref = a_msg.GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> l_fs;
        l_ref->ListFields(a_msg, &l_fs);
        for (std::vector<const google::protobuf::FieldDescriptor*>::const_iterator i_f = l_fs.begin();
             i_f != l_fs.end();
             ++i_f)
        {
                const google::protobuf::FieldDescriptor* l_f = *i_f;

                switch (l_f->label())
                {
                case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
                case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = convert_single_field(l_v, l_alx, l_ref, l_f, a_msg);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                case google::protobuf::FieldDescriptor::LABEL_REPEATED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = convert_repeated_field(l_v, l_alx, l_ref, l_f, a_msg);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                default:
                {
                        JSPB_PERROR("unknown label in field '%s': %d", l_f->full_name().c_str(), l_f->label());
                        return JSPB_ERROR;
                }
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a protobuf message to a json object, storing the
//:           result in a std::string.
//: \return:  TODO
//: \param:   msg the protobuf message to convert
//: \param:   value json object to hold the converted value
//: ----------------------------------------------------------------------------
int32_t convert_to_json(std::string& ao_str,
                        const google::protobuf::Message& a_msg)
{
        // put on the heap...
        rapidjson::Document *l_js = new rapidjson::Document();
        rapidjson::StringBuffer l_strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> l_js_writer(l_strbuf);
        int32_t l_s;
        l_s = convert_to_json(*l_js, a_msg);
        if(l_s != JSPB_OK)
        {
                goto done;
        }
        // write back to str
        (*l_js).Accept(l_js_writer);
        // TODO -can eliminate copy here???
        ao_str = l_strbuf.GetString();
done:
        if(l_js)
        {
                delete l_js;
                l_js = NULL;
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t update_from_json(google::protobuf::Message& ao_msg,
                         const rapidjson::Value &a_val)
{
        if(a_val.IsObject())
        {
                JSPB_PERROR("error json not an object");
                return JSPB_ERROR;
        }
        // Walk through the json members and insert them into the protobuf.
        const google::protobuf::Reflection* l_ref = ao_msg.GetReflection();
        const google::protobuf::Descriptor* l_des = ao_msg.GetDescriptor();

        // ---------------------------------------
        // Iterate over objects...
        // ---------------------------------------
        for (rapidjson::Value::ConstMemberIterator i_m = a_val.MemberBegin();
             i_m != a_val.MemberEnd();
             ++i_m)
        {
                const char *i_k = i_m->name.GetString();
                const google::protobuf::FieldDescriptor* l_f = l_des->FindFieldByName(i_k);
                if (0 == l_f)
                {
                        JSPB_PERROR("json field '%s' not found in message", i_k);
                        return JSPB_ERROR;
                }

                //const rapidjson::GenericValue<rapidjson::UTF8<> >::ConstObject &i_v = i_m->value.GetObject();

                const rapidjson::Value &i_v = i_m->value;

                switch (l_f->label())
                {
                case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
                case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = update_single_field(ao_msg,
                                                  l_ref,
                                                  l_des,
                                                  l_f,
                                                  i_v);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                case google::protobuf::FieldDescriptor::LABEL_REPEATED:
                {
                        // make sure the json value is an array
                        if(!i_m->value.IsArray())
                        {
                                JSPB_PERROR("expecting json array for field '%s'", l_f->full_name().c_str());
                                return JSPB_ERROR;
                        }
                        // the json array will completely replace this field
                        l_ref->ClearField(&ao_msg, l_f);
                        int32_t l_s = JSPB_OK;
                        l_s = update_repeated_field(ao_msg,
                                                    l_ref,
                                                    l_des,
                                                    l_f,
                                                    i_v);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                default:
                {
                        JSPB_PERROR("unknown label in field '%s': %d", l_f->full_name().c_str(), l_f->label());
                        return JSPB_ERROR;
                }
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a json object to a protobuf message, reading the
//:           json value from a Json::Value object.
//: \return:  TODO
//: \param:   value json object to convert
//: \param:   message protobuf message to hold the converted value
//: ----------------------------------------------------------------------------
int32_t update_from_json(google::protobuf::Message& ao_msg,
                         const rapidjson::Document& a_js)
{
        if(a_js.IsObject())
        {
                JSPB_PERROR("error json not an object");
                return JSPB_ERROR;
        }
        // Walk through the json members and insert them into the protobuf.
        const google::protobuf::Reflection* l_ref = ao_msg.GetReflection();
        const google::protobuf::Descriptor* l_des = ao_msg.GetDescriptor();

        // ---------------------------------------
        // Iterate over objects...
        // ---------------------------------------
        for (rapidjson::Value::ConstMemberIterator i_m = a_js.MemberBegin();
             i_m != a_js.MemberEnd();
             ++i_m)
        {
                const char *i_k = i_m->name.GetString();
                const google::protobuf::FieldDescriptor* l_f = l_des->FindFieldByName(i_k);
                if (0 == l_f)
                {
                        JSPB_PERROR("json field '%s' not found in message", i_k);
                        return JSPB_ERROR;
                }

                //const rapidjson::GenericValue<rapidjson::UTF8<> >::ConstObject &i_v = i_m->value.GetObject();

                const rapidjson::Value &i_v = i_m->value;

                switch (l_f->label())
                {
                case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
                case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
                {
                        int32_t l_s = JSPB_OK;
                        l_s = update_single_field(ao_msg,
                                                  l_ref,
                                                  l_des,
                                                  l_f,
                                                  i_v);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                case google::protobuf::FieldDescriptor::LABEL_REPEATED:
                {
                        // make sure the json value is an array
                        if(!i_m->value.IsArray())
                        {
                                JSPB_PERROR("expecting json array for field '%s'", l_f->full_name().c_str());
                                return JSPB_ERROR;
                        }
                        // the json array will completely replace this field
                        l_ref->ClearField(&ao_msg, l_f);

                        int32_t l_s = JSPB_OK;
                        l_s = update_repeated_field(ao_msg,
                                                    l_ref,
                                                    l_des,
                                                    l_f,
                                                    i_v);
                        if(l_s != JSPB_OK)
                        {
                                return JSPB_ERROR;
                        }
                        break;
                }
                default:
                {
                        JSPB_PERROR("unknown label in field '%s': %d", l_f->full_name().c_str(), l_f->label());
                        return JSPB_ERROR;
                }
                }
        }
        return JSPB_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a json object to a protobuf message, reading the
//:           json value from a std::string.
//: \return:  TODO
//: \param:   value json object to convert
//: \param:   message protobuf message to hold the converted value
//: ----------------------------------------------------------------------------
int32_t update_from_json(google::protobuf::Message& ao_msg,
                         const char *a_buf,
                         uint32_t a_len)
{
        // put on the heap...
        rapidjson::Document *l_js = new rapidjson::Document();
        l_js->Parse(a_buf, a_len);
        if(!l_js->IsObject())
        {
                JSPB_PERROR("error parsing json");
                return JSPB_ERROR;
        }
        int32_t l_s;
        l_s = update_from_json(ao_msg, *l_js);
        if(l_js)
        {
                delete l_js;
                l_js = NULL;
        }
        return l_s;
}

//: ----------------------------------------------------------------------------
//: \details: Convert a json object to a protobuf message, reading the
//:           json value from a std::string.
//: \return:  TODO
//: \param:   value json object to convert
//: \param:   message protobuf message to hold the converted value
//: ----------------------------------------------------------------------------
int32_t update_from_json(google::protobuf::Message& ao_msg,
                         const std::string& a_str)
{
        return update_from_json(ao_msg, a_str.c_str(), a_str.length());
}

//: ----------------------------------------------------------------------------
//: \details: Get last error
//: \return:  Last error reason
//: ----------------------------------------------------------------------------
const char * get_err_msg(void)
{
        return g_err_msg;
}

} // namespace

