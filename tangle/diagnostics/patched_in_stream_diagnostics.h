#pragma once

#include "error.h"
#include "patched_in_stream.h"

class patched_in_stream_diagnostics : public patched_in_stream::diagnostics
{
    error_manager &web_err, &change_err;

public:
    patched_in_stream_diagnostics (error_manager &web_err, error_manager &change_err)
    : web_err (web_err), change_err (change_err)
    {}

    void on_lines_dont_match (size_t non_matching_lines) override 
    { change_err.err_print ("! Hmm... {} of the preceding lines failed to match", non_matching_lines); }


    void on_web_file_ended_during_change () override 
    { web_err.err_print ("! WEB file ended during a change"); }
};

