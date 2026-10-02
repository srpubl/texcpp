#pragma once

#include "error.h"
#include "module_name_scanner.h"
#include "name_scanner.h"
#include "patched_in_stream.h"

class input_token_stream 
{
public:
    struct diagnostics 
    : public virtual name_scanner::diagnostics
    , public virtual module_name_scanner::diagnostics
    {
        virtual void on_new_major_section () = 0;
        virtual void on_section_end_in_comment (error_manager &err) = 0;
        virtual void on_input_end_in_comment (error_manager &err) = 0;
        virtual void on_extra_brace (error_manager &err) = 0;
        virtual void on_improper_marker (error_manager &err) = 0;

        virtual ~diagnostics () = default;
    };

private:
    diagnostics &_diagnose;
    patched_in_stream &_in_str;
    name_scanner _name_scnr;
    module_name_scanner _mod_name_scnr;

    bool _scanning_hex = false;

public:
    input_token_stream (
        diagnostics &diagnose, 
        patched_in_stream &in_str,
        name_manager &name_mgr) 
    : _diagnose (diagnose)
    , _in_str (in_str)
    , _name_scnr (diagnose, name_mgr)
    , _mod_name_scnr (diagnose, name_mgr) 
    {}

    void
    initialize ()
    {
        _scanning_hex = false;
    }

    auto & current_identifier (ilk_value type) { return _name_scnr.retrieve_name (type); }
    auto & current_module_name () { return _mod_name_scnr.current_module_name (); }

    char8_t get ();
    char8_t get_next_control_code ();

private:
    void skip_comment ();
    char8_t control_code (char8_t);

    bool
    compress_if (char8_t &c, char8_t match, char8_t compressed)
    {
        if (_in_str.peek () != match)
            return false;

        if (!_in_str.eol ())
        {
            c = compressed;
            _in_str.advance ();
        }

        return true;
    }
};

