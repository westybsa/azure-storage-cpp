// -----------------------------------------------------------------------------------------
// <copyright file="response_parsers.cpp" company="Microsoft">
//    Copyright 2013 Microsoft Corporation
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//      http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// </copyright>
// -----------------------------------------------------------------------------------------

#include "stdafx.h"
#include "wascore/protocol.h"
#include "wascore/constants.h"
#include "wascore/resources.h"

namespace wa { namespace storage { namespace protocol {

    void preprocess_response(const web::http::http_response& response, operation_context context)
    {
        preprocess_response<char>(0, response, context);
    }

    /*
    void preprocess_table_response(const web::http::http_response& response, operation_context context)
    {
        switch (response.status_code())
        {
        case web::http::status_codes::OK:
        case web::http::status_codes::Created:
        case web::http::status_codes::Accepted:
        case web::http::status_codes::NoContent:
        case web::http::status_codes::PartialContent:
            break;

        default:
            utility::ostringstream_t str;
            str << U("Status code: ") << response.status_code();
            throw storage_exception(utility::conversions::to_utf8string(str.str()));
        }
    }
    */

    void check_stream_length_and_md5(utility::size64_t length, const utility::string_t& content_md5, const core::ostream_descriptor& descriptor)
    {
        if (length != descriptor.length())
        {
            throw storage_exception(utility::conversions::to_utf8string(error_incorrect_length));
        }

        if (!content_md5.empty() && !descriptor.content_md5().empty() && (content_md5 != descriptor.content_md5()))
        {
            throw storage_exception(utility::conversions::to_utf8string(error_md5_mismatch));
        }
    }

    utility::string_t get_header_value(const web::http::http_headers& headers, const utility::string_t& header)
    {
        utility::string_t value;
        headers.match(header, value);
        return value;
    }

    utility::string_t get_header_value(const web::http::http_response& response, const utility::string_t& header)
    {
        return get_header_value(response.headers(), header);
    }

    utility::string_t parse_etag(const web::http::http_response& response)
    {
        return get_header_value(response, web::http::header_names::etag);
    }

    utility::datetime parse_last_modified(const utility::string_t& value)
    {
        return utility::datetime::from_string(value, utility::datetime::date_format::RFC_1123);
    }

    utility::datetime parse_last_modified(const web::http::http_response& response)
    {
        utility::string_t value;
        if (response.headers().match(web::http::header_names::last_modified, value))
        {
            return parse_last_modified(value);
        }
        else
        {
            return utility::datetime();
        }
    }

    utility::string_t parse_lease_id(const web::http::http_response& response)
    {
        return get_header_value(response, ms_header_lease_id);
    }

    lease_status parse_lease_status(const utility::string_t& value)
    {
        if (value == header_value_locked)
        {
            return lease_status::locked;
        }
        else if (value == header_value_unlocked)
        {
            return lease_status::unlocked;
        }
        else
        {
            return lease_status::unspecified;
        }
    }

    lease_status parse_lease_status(const web::http::http_response& response)
    {
        return parse_lease_status(get_header_value(response, ms_header_lease_status));
    }

    lease_state parse_lease_state(const utility::string_t& value)
    {
        if (value == header_value_lease_available)
        {
            return lease_state::available;
        }
        else if (value == header_value_lease_breaking)
        {
            return lease_state::breaking;
        }
        else if (value == header_value_lease_broken)
        {
            return lease_state::broken;
        }
        else if (value == header_value_lease_expired)
        {
            return lease_state::expired;
        }
        else if (value == header_value_lease_leased)
        {
            return lease_state::leased;
        }
        else
        {
            return lease_state::unspecified;
        }
    }

    lease_state parse_lease_state(const web::http::http_response& response)
    {
        return parse_lease_state(get_header_value(response, ms_header_lease_state));
    }

    lease_duration parse_lease_duration(const utility::string_t& value)
    {
        if (value == header_value_lease_infinite)
        {
            return lease_duration::infinite;
        }
        else if (value == header_value_lease_fixed)
        {
            return lease_duration::fixed;
        }
        else
        {
            return lease_duration::unspecified;
        }
    }

    lease_duration parse_lease_duration(const web::http::http_response& response)
    {
        return parse_lease_duration(get_header_value(response, ms_header_lease_duration));
    }

    std::chrono::seconds parse_lease_time(const web::http::http_response& response)
    {
        utility::string_t value;
        if (response.headers().match(ms_header_lease_time, value))
        {
            int64_t seconds = utility::conversions::scan_string<int64_t>(value);
            return std::chrono::seconds(seconds);
        }
        else
        {
            return std::chrono::seconds();
        }
    }

    int parse_approximate_messages_count(const web::http::http_response& response)
    {
        utility::string_t value;
        if (response.headers().match(ms_header_approximate_messages_count, value))
        {
            return utility::conversions::scan_string<int>(value);
        }

        return -1;
    }

    utility::string_t parse_pop_receipt(const web::http::http_response& response)
    {
        utility::string_t value;
        if (response.headers().match(ms_header_pop_receipt, value))
        {
            return value;
        }

        return utility::string_t();
    }

    utility::datetime parse_next_visible_time(const web::http::http_response& response)
    {
        utility::string_t value;
        if (response.headers().match(ms_header_time_next_visible, value))
        {
            return utility::datetime::from_string(value, utility::datetime::RFC_1123);
        }

        return utility::datetime();
    }

    cloud_metadata parse_metadata(const web::http::http_response& response)
    {
        cloud_metadata metadata;

        auto& headers = response.headers();
        for (auto iter = headers.begin(); iter != headers.end(); ++iter)
        {
            auto key = iter->first;
            if ((key.size() > ms_header_metadata_prefix.size()) &&
                std::equal(ms_header_metadata_prefix.cbegin(), ms_header_metadata_prefix.cend(), key.cbegin()))
            {
                metadata.insert(std::make_pair(key.substr(ms_header_metadata_prefix.size()), iter->second));
            }
        }

        return metadata;
    }

}}} // namespace wa::storage::protocol
