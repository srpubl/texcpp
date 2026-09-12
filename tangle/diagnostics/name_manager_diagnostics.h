#pragma once

#include "error.h"
#include "name_manager.h"

class name_manager_diagnostics : public name_manager::diagnostics
{
    error_manager &err;

public:
    name_manager_diagnostics (error_manager &err) : err (err) {}

    void on_already_appeared () override { err.err_print ("! This identifier has already appeared"); }
    void on_defined_before () override { err.err_print ("! This identifier was defined before"); }
    void on_incompatible () override { err.err_print ("! Incompatible section names"); }
    void on_no_match () override { err.err_print ("! Name does not match"); }
    void on_too_many_matches () override { err.err_print ("! Ambiguous prefix"); }

    void
    on_id_conflict (std::u8string_view id) override
    {
        err.terminal ().print_nl ("! Identifier conflict with ");
        err.terminal ().print (id);
        err.error ();
    }
};


