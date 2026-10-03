#pragma once

#include "config.h"
#include "error.h"
#include "name_manager.h"
#include "patched_in_stream.h"
#include <optional>

class module_name_scanner
{
public:
    struct diagnostics
    {
        virtual void on_too_long (error_manager &err, std::u8string_view name) = 0;
        virtual void on_input_ended (error_manager &err) = 0;
        virtual void on_missing_end (error_manager &err) = 0;

        virtual ~diagnostics () = default;
    };

private:
    diagnostics &diagnose;
    name_manager &name_mgr;

    util::static_vector <char8_t> mod_name;

public:
    module_name_scanner (diagnostics &diagnose, name_manager &name_mgr) 
    : diagnose (diagnose), name_mgr (name_mgr) 
    { 
        mod_name.reserve (config::longest_name);
    }

    void
    scan_module_name (patched_in_stream &in_str)
    {
        read_module_name (in_str);

        if (mod_name.size () > config::longest_name - 2)
        {
            diagnose.on_too_long (in_str.err (), {mod_name.begin (),  mod_name.begin() + 25});
        }

        if (mod_name.size () > 0 && mod_name.back () == u8' ')
        {
            mod_name.pop_back ();
        }
    }

    auto
    current_module_name () -> name_t::optional_reference
    {        
        if (mod_name.size () < 4)
            return std::nullopt;

        if (std::equal (mod_name.end () - 3, mod_name.end (), u8"..."))
            return name_mgr.lookup_prefix ({mod_name.begin (), mod_name.end () - 3});
                
        return name_mgr.lookup_module ({mod_name.begin (), mod_name.end ()});
    }

    auto current_module_id () { return name_mgr.index_of (*current_module_name ()); }


private:
    void
    read_module_name (patched_in_stream &in_str);
};

