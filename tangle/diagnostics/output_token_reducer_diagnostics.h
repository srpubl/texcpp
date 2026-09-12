#pragma once

#include "error.h"
#include "output_token_reducer.h"

class output_token_reducer_diagnostics : public output_token_reducer::diagnostics
{
    error_manager &err;

public:
    output_token_reducer_diagnostics (error_manager &err) : err (err) {}

    void on_constant_too_big    () override {  err.err_print ("! Constant too big"); }
    void on_extra_closing_brace () override {  err.err_print ("! Extra @}}"); }
    void on_fraction_too_long   () override {  err.err_print ("! Fraction too long"); }
    void on_string_too_long     () override {  err.err_print ("! String too long"); }
    void on_verbatim_too_long   () override {  err.err_print ("! Verbatim string too long"); }
    
    void on_invalid_ascii (char8_t c) override 
    { err.err_print ("! Can't output ASCII code {}", static_cast<uint8_t> (c));}
};


