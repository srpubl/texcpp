#pragma once

#include "name_scanner.h"

struct name_scanner_diagnostics : public name_scanner::diagnostics
{
    void on_string_without_end (error_manager &err) override 
    { err.err_print ("! String constant didn't end"); }
    
    void on_missing_double_escape (error_manager &err) override 
    { err.err_print ("! Double @ sign missing"); }

};



