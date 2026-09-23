#pragma once

#include "error.h"
#include "in_stream.h"

class in_stream_diagnostics : public in_stream::diagnostics
{
    error_manager &err;

public:
    in_stream_diagnostics (error_manager &err) : err (err) {}

    void
    on_line_too_long () override
    { err.err_print ("! Input line too long"); }
};


