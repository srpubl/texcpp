#pragma once

#include "error.h"
#include "out_processor.h"

class out_processor_diagnostics : public out_processor::diagnostics
{
    error_manager &err;

public:
    out_processor_diagnostics (error_manager &err) : err (err) {}

    void
    on_missing_sign_between_numbers () override
    { err.err_print ("! Two numbers occurred without a sign between them"); }
};


