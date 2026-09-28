#pragma once

#include "change_stream.h"
#include "in_stream_diagnostics.h"

struct change_stream_diagnostics : public in_stream_diagnostics, public change_stream::diagnostics
{
    change_stream_diagnostics (error_manager &err) 
    : in_stream::diagnostics (err)
    , in_stream_diagnostics (err)
    {}

    void on_missing_x () override { err.err_print ("! Where is the matching @x?"); }
    void on_missing_y () override { err.err_print ("! Where is the matching @y?"); }
    void on_missing_z () override { err.err_print ("! Where is the matching @z?"); }
    void on_ended_after_x () override { err.err_print ("! Change file ended after @x"); }
    void on_ended_before_y () override { err.err_print ("\n! Change file ended before @y"); }            
    void on_ended_without_z () override { err.err_print ("\n! Change file ended without @z"); }            
    void on_extra_change () override { err.err_print ("! Change file entry did not match"); }
};


