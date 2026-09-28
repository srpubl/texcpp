#pragma once

#include "module_name_scanner.h"

class module_name_scanner_diagnostics : public module_name_scanner::diagnostics
{
    virtual void 
    on_too_long (error_manager &err, std::u8string_view name) override 
    {
        err.terminal ().print_nl ("! Section name too long: ");
        err.terminal ().print (name);
        err.terminal ().print ("...");
        err.mark_harmless ();
    }

    virtual void on_input_ended (error_manager &err) override 
    { err.err_print ("! Input has ended in section name"); }
    
    virtual void on_missing_end (error_manager &err) override 
    { err.err_print ("! Section name didn't end"); }

};


