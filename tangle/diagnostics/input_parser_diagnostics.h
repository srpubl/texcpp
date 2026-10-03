#pragma once

#include "error.h"
#include "input_parser.h"
#include "input_token_stream_diagnostics.h"

#include "tokens.h"

class input_parser_diagnostics 
: public virtual input_parser::diagnostics
, public virtual input_token_stream_diagnostics
{

public:
    input_parser_diagnostics (terminal &term)
    : input_token_stream_diagnostics(term)
    {}

    void on_improper_numeric_definition (error_manager &err) override 
    { err.err_print ("! Improper numeric definition will be flushed"); }

    void on_semicolon_in_numeric_definition (error_manager &err) override 
    { err.err_print ("! Omit semicolon in numeric definition"); }

    void on_value_too_big_in_numeric_definition (error_manager &err, int value) override 
    { err.err_print ("! Value too big: {}", value);}

    void on_unbalanced_parentheses (error_manager &err, int balance) override 
    {         
        if (balance == 1)
        {
            err.err_print ("! Missing )");
        }
        else
        {
            err.err_print ("! Missing {} )'s", balance);
        }
    }

    void on_missing_second_equal_sign_in_macro_definition (error_manager &err) override
    { err.err_print ("! Use == for macros"); }

    void on_no_valid_identifier_for_definition (error_manager &err) override
    { err.err_print ("! Definition flushed must start with identifier of length > 1"); }

    void on_missing_equal_sign_starting_pascal (error_manager &err) override
    { err.err_print ("! Pascal text flushed, = sign is missing"); }

    void on_extra_parenthesis (error_manager &err) override
    { err.err_print ("! Extra )"); }

    void on_extra_control_code_in_replacement (error_manager &err, char8_t code) override
    {
        char ch;
        switch (code)
        {
        case definition: ch = 'd'; break;
        case format:     ch = 'f'; break;
        case begin_pascal: ch = 'p'; break;
        }

        err.err_print ("! @{} is ignored in Pascal text", ch);
    }
};

