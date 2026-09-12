#pragma once

#include "output_token_stream.h"
#include "out_processor.h"

class output_token_reducer 
{
public:
    struct diagnostics 
    {
        virtual void on_constant_too_big    () = 0;
        virtual void on_extra_closing_brace () = 0;
        virtual void on_fraction_too_long   () = 0;
        virtual void on_string_too_long     () = 0;
        virtual void on_verbatim_too_long   () = 0;

        virtual void on_invalid_ascii (char8_t) = 0;
    };

private:
    output_token_stream &output_token_str;
    out_processor &out_proc;
    int &pool_check_sum;
    diagnostics &diagnose;

    /// returns next token after macro expansion
    int last_char;

    /// current depth of @{...@} nesting
    uint8_t _brace_level;

public:
    output_token_reducer (
        output_token_stream &output_token_str, 
        out_processor &out_proc, 
        int &pool_check_sum,
        diagnostics &diagnose)
    : output_token_str (output_token_str)
    , out_proc (out_proc)
    , pool_check_sum(pool_check_sum)
    , diagnose (diagnose)
    {}

    void
    initialize () 
    {
        last_char = -1;
        _brace_level = 0;
    }

    auto brace_level () { return _brace_level; }

    void
    send_the_output ();

private:
    char8_t
    get_output ()
    {
        if (last_char < 0)
            return static_cast<char8_t> (output_token_str.get_output () & 0xFF);

        auto res = static_cast<char8_t> (last_char & 0xFF);
        last_char        = -1;
        return res;
    }

    void
    put_back_output (char8_t c) { last_char = c; }

    char8_t
    peek_output ()
    {
        auto c = get_output ();
        put_back_output (c);
        return c;
    }

    void send_output_identifier (std::u8string_view id);
    void send_output_module_number (int module_number);
    void send_output_string ();
    void send_output_verbatim_string ();
    void send_out_number (ascii_code_t cur_char, int base, int limit, bool (*is_valid) (ascii_code_t));
    void finish_real_constant (bool);
    void send_output_dot ();
    void send_output_begin_comment ();
    void send_output_end_comment ();
    void send_output_one_char ();
};