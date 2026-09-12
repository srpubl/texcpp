#pragma once

#include "error.h"
#include "output_token_stream.h"

class output_token_stream_diagnostics : public output_token_stream::diagnostics
{
    error_manager &err;

public:
    output_token_stream_diagnostics (error_manager &err) : err (err) {}

    void
    on_stack_overflow () override
    {  err.overflow ("stack"); }

    void
    on_name_not_found (std::u8string_view id) override
    {
        err.terminal ().print_nl ("! Not present: <");
        err.terminal ().print (id);
        err.terminal ().print ('>');
        err.error ();
    }

    void
    on_missing_parameter (std::u8string_view id) override
    {
        err.terminal ().print_nl ("! No parameter given for ");
        err.terminal ().print (id);
        err.error ();
    }

    void 
    on_invalid_ilk () override
    { err.confusion ("output"); }
};


