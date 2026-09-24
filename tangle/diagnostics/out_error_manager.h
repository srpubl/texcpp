#pragma once

#include "error.h"
#include "out_buffer.h"

class out_error_manager : public error_manager
{
    out_buffer *out_buf = nullptr;

public:
    using error_manager::error_manager;

    void set_buffer (out_buffer *buf) { out_buf = buf; }

protected:
    void print_error_location () override
    {
        terminal ().print_ln (". (l.{})", out_buf -> current_line ());
        terminal ().print (out_buf -> temporary_view ());
        terminal ().print ("... ");
    }
};

