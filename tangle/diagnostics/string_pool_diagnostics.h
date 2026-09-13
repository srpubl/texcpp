#pragma once

#include "error.h"
#include "string_pool.h"

class string_pool_diagnostics : public string_pool::diagnostics 
{
    terminal &term;
    error_manager &err;

public:
    string_pool_diagnostics (terminal &term, error_manager &err) : term (term), err (err)
    {}

    void on_string_too_long () override { err.err_print ("! Preprocessed string is too long"); }
    
    void 
    on_summary (config::index_t count) override
    {
        term.print_nl ("{} strings written to string pool file.", count);
    } 
};
