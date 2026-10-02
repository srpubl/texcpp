#pragma once

#include "input_token_stream.h"
#include "module_name_scanner_diagnostics.h"
#include "name_scanner_diagnostics.h"

class input_token_stream_diagnostics 
: public virtual input_token_stream::diagnostics
, public virtual name_scanner_diagnostics
, public virtual module_name_scanner_diagnostics
{
    terminal &term;
    size_t &module_count;

public:
    input_token_stream_diagnostics (terminal &term, size_t &module_count) 
    : term (term), module_count (module_count) {}

    void 
    on_new_major_section () override
    {
        term.print ("*{}", module_count + 1);
        term.update ();
    }

    void on_section_end_in_comment (error_manager &err) override 
    { err.err_print ("! Section ended in mid-comment"); }
    
    void on_input_end_in_comment (error_manager &err) override 
    { err.err_print ("! Input ended in mid-comment"); }
    
    void on_improper_marker (error_manager &err) override 
    { err.err_print ("! Improper @ within control text"); }

    void on_extra_brace (error_manager &err) override 
    { err.err_print ("! Extra }}"); }    
};


