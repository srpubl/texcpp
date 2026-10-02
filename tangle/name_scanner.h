#pragma once

#include "error.h"
#include "patched_in_stream.h"
#include "name_manager.h"

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
    name_manager &name_mgr;

    size_t 
    double_chars     = {};
    
    std::u8string_view
    content       = {};

public:
    name_scanner (diagnostics & diagnose, name_manager &name_mgr) 
    : diagnose (diagnose), name_mgr (name_mgr) {}

    char8_t
    scan_identifier (patched_in_stream &in_str);

    char8_t
    scan_preprocessed_string (patched_in_stream &in_str);

    auto &
    retrieve_name (ilk_value type)  { return name_mgr.lookup (type, content, double_chars); }
};

