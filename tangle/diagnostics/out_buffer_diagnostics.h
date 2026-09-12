#pragma once

#include "error.h"
#include "out_buffer.h"
#include "terminal.h"

class out_buffer_diagnostics : public out_buffer::diagnostics
{
    terminal &term;
    error_manager &err;

public:
    out_buffer_diagnostics (terminal &term, error_manager &err) : term (term), err (err) {}

    void
    on_new_line (int line) override
    {
        if (line % 100 == 0)
        {
            term.print ('.');
            if (line % 500 == 0)
            {
                term.print ("{}", line);
            }
            term.update ();
        }
    }

    void
    on_line_truncated () override
    { err.err_print ("! Long line must be truncated"); }
};


