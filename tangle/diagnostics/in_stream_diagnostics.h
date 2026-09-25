#pragma once

#include "error.h"
#include "in_stream.h"

struct in_stream_diagnostics : public virtual in_stream::diagnostics
{
    using in_stream::diagnostics::diagnostics;

    void
    on_line_too_long () override
    { err.err_print ("! Input line too long"); }
};


