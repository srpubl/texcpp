#pragma once

#include "config.h"

#include "error.h"
#include "in_stream.h"
#include "name_manager.h"
#include "string_pool.h"

class name_scanner 
{
public:
    struct diagnostics 
    {
        virtual void on_string_without_end (error_manager &) = 0;
        virtual void on_missing_double_escape (error_manager &) = 0;

        virtual ~diagnostics () = default;
    };

private:
    diagnostics &diagnose;
    string_pool &str_pool;
    name_manager &name_mgr;

    size_t 
    double_chars     = {};
    
    std::u8string_view
    content       = {};

public:
    name_scanner (diagnostics & diagnose, string_pool &str_pool, name_manager &name_mgr) 
    : diagnose (diagnose), str_pool (str_pool), name_mgr (name_mgr) {}

    auto
    add_string_to_pool () -> config::index_t
    {
        if (content.length () - double_chars == 2)  // single-character string
            return content [1];

        auto length = content.length () - (double_chars + 1);

        return str_pool.add (content, length);
    }

    char8_t
    scan_identifier (in_stream &in_str);

    char8_t
    scan_preprocessed_string (in_stream &in_str);

    auto &
    retrieve_name (ilk_value type)  { return name_mgr.lookup (type, content); }
};

