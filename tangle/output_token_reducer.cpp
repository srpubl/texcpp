#include "output_token_reducer.h"
#include "tokens.h"

// section 113

#define DEF_CASE(x) case x:

#define DEF_2_CASES_FROM(x) DEF_CASE (x) DEF_CASE (x + 1)
#define DEF_4_CASES_FROM(x) DEF_2_CASES_FROM (x) DEF_2_CASES_FROM (x + 2)
#define DEF_8_CASES_FROM(x) DEF_4_CASES_FROM (x) DEF_4_CASES_FROM (x + 4)
#define DEF_10_CASES_FROM(x) DEF_8_CASES_FROM (x) DEF_2_CASES_FROM (x + 8)
#define DEF_16_CASES_FROM(x) DEF_8_CASES_FROM (x) DEF_8_CASES_FROM (x + 8)
#define DEF_26_CASES_FROM(x) DEF_16_CASES_FROM (x) DEF_10_CASES_FROM (x + 16)

#define DEF_CASE_OF(x, i) case x [i]:

#define DEF_2_CASES_OF(x, i) DEF_CASE_OF (x, i) DEF_CASE_OF (x, i + 1)
#define DEF_4_CASES_OF(x, i) DEF_2_CASES_OF (x, i) DEF_2_CASES_OF (x, i + 2)
#define DEF_8_CASES_OF(x, i) DEF_4_CASES_OF (x, i) DEF_4_CASES_OF (x, i + 4)
#define DEF_10_CASES_OF(x, i) DEF_8_CASES_OF (x, i) DEF_2_CASES_OF (x, i + 8)
#define DEF_16_CASES_OF(x, i) DEF_8_CASES_OF (x, i) DEF_8_CASES_OF (x, i + 8)
#define DEF_26_CASES_OF(x, i) DEF_16_CASES_OF (x, i) DEF_10_CASES_OF (x, i + 16)

void
output_token_reducer::send_output_dot ()
{
    switch (peek_output ())
    {
    case u8'.':
        get_output ();  // consume it
        out_proc.process_string (u8"..");
        break;

        DEF_10_CASES_FROM ('0')
        finish_real_constant (true);
        break;

    default: out_proc.process_single_char (u8'.');
    }
}

void
output_token_reducer::send_output_begin_comment ()
{ out_proc.process_single_char (_brace_level++ == 0 ? u8'{' : u8'['); }

void
output_token_reducer::send_output_end_comment ()
{
    if (_brace_level > 0)
    {
        out_proc.process_single_char (--_brace_level == 0 ? u8'}' : u8']');
    }
    else
    {
        diagnose.on_extra_closing_brace();
    }
}

void
output_token_reducer::send_output_one_char ()
{
    constexpr auto single_char_cases = u8"!\"#$%&()*,/:;<=>?@[\\]^_`{|"sv;

    ascii_code_t   cur_char          = get_output ();
    switch (cur_char)
    {
    case 0:
        break;

        DEF_10_CASES_FROM ('0')
        send_out_number (cur_char, 10, 0xCCCCCCC, is_digit);
        switch (peek_output ())
        {
        case u8'e':
        case u8'E': finish_real_constant (false);
        }
        break;

        DEF_26_CASES_FROM (u8'A')
        out_proc.process_identifier ({&cur_char, 1});
        break;

        DEF_26_CASES_FROM (u8'a')
        cur_char -= 0x20;
        out_proc.process_identifier ({&cur_char, 1});
        break;

        static_assert (single_char_cases.length () == 26);
        DEF_26_CASES_OF (single_char_cases, 0)
        out_proc.process_single_char (cur_char);
        break;

    case u8'\''          : send_output_string (); break;
    case u8'.'           : send_output_dot (); break;
    case u8'+'           : out_proc.process_sign (+1); break;
    case u8'-'           : out_proc.process_sign (-1); break;
    case and_sign        : out_proc.process_identifier (u8"AND"); break;
    case not_sign        : out_proc.process_identifier (u8"NOT"); break;
    case set_element_sign: out_proc.process_identifier (u8"IN"); break;
    case or_sign         : out_proc.process_identifier (u8"OR"); break;
    case not_equal       : out_proc.process_string (u8"<>"); break;
    case greater_or_equal: out_proc.process_string (u8">="); break;
    case equivalence_sign: out_proc.process_string (u8"=="); break;
    case left_arrow      : out_proc.process_string (u8":="); break;
    case less_or_equal   : out_proc.process_string (u8"<="); break;
    case double_dot      : out_proc.process_string (u8".."); break;
    case begin_comment   : send_output_begin_comment (); break;
    case end_comment     : send_output_end_comment (); break;
    case identifier      : send_output_identifier (output_token_str.extra_identifier ()); break;
    case module_number   : send_output_module_number (output_token_str.extra ()); break;
    case verbatim        : send_output_verbatim_string (); break;
    case octal           : send_out_number (u8'0', 8, 0x10000000, is_octal); break;
    case hex             : send_out_number (u8'0', 16, 0x8000000, is_hex); break;
    case number          : out_proc.process_value (output_token_str.extra ()); break;
    case check_sum       : out_proc.process_value (pool_check_sum); break;
    case force_line      : out_proc.force_line_break (); break;

    case join:
        out_proc.process_fraction ({});
        out_proc.ensure_no_line_break ();
        break;

    default: diagnose.on_invalid_ascii (cur_char); break;
    }
}

void
output_token_reducer::send_the_output ()
{
    while (output_token_str.has_more() || last_char >= 0) { send_output_one_char (); }
}

// section 114
// section 115 incorporated in section 113
// section 116

void
output_token_reducer::send_output_identifier (std::u8string_view id)
{
    auto   buffer = std::array<ascii_code_t, config::max_id_length> {};
    size_t k      = 0;

    for (auto ch : id)
    {
        if (ch != u8'_')
        {
            buffer [k++] = ch >= u8'a' ? ch - 0x20 : ch;
        }

        if (k == buffer.size ())
            break;
    }

    out_proc.process_identifier ({buffer.data (), k});
}

// Section 117

void
output_token_reducer::send_output_string ()
{
    auto   buffer = std::array<ascii_code_t, config::line_length> {};
    size_t k      = 0;
    buffer [0]    = u8'\'';
    ascii_code_t ch;
    do
    {
        if (k < config::line_length - 1)
        {
            ++k;
        }
        buffer [k] = ch = get_output ();
    }
    while (ch != u8'\'' && output_token_str.has_more ());

    if (k == config::line_length - 1)
    {
        diagnose.on_string_too_long ();
    }
    out_proc.process_string ({buffer.data (), k + 1});
    if (peek_output () == '\'')
    {
        out_proc.ensure_no_line_break ();
    }
}

// section 118

void
output_token_reducer::send_output_verbatim_string ()
{
    auto         buffer = std::array<ascii_code_t, config::line_length> {};
    size_t       k      = 0;
    ascii_code_t ch;
    do
    {
        buffer [k] = ch = get_output ();
        if (k < config::line_length - 1)
        {
            ++k;
        }
    }
    while (ch != verbatim && output_token_str.has_more());

    if (k == config::line_length - 1)
    {
        diagnose.on_verbatim_too_long ();
    }
    out_proc.process_string ({buffer.data (), (size_t) k - 1});
}

// section 119

void
output_token_reducer::send_out_number (
    ascii_code_t cur_char, int base, int limit, bool (*is_valid) (ascii_code_t))
{
    int n = 0;
    do
    {
        int digit_value = (cur_char >= u8'A') ? (cur_char - u8'A' + 10) : (cur_char - u8'0');
        if (n >= limit)
        {
            diagnose.on_constant_too_big ();
        }
        else
        {
            n = base * n + digit_value;
        }
        cur_char = get_output ();
    }
    while (is_valid (cur_char));
    put_back_output (cur_char);
    out_proc.process_value (n);
}

// section 120

void
output_token_reducer::finish_real_constant (bool start_with_dot)
{
    ascii_code_t cur_char = get_output ();
    auto         buffer   = std::array<ascii_code_t, config::line_length> {};
    size_t       k        = 0;
    if (start_with_dot)
    {
        buffer [k++] = u8'.';
    }

    do
    {
        if (k < config::line_length)
        {
            ++k;
        }

        auto last_char = cur_char;
        buffer [k - 1] = cur_char;
        cur_char       = get_output ();

        if (last_char == u8'E' && (cur_char == u8'+' || cur_char == u8'-'))
        {
            if (k < config::line_length)
            {
                ++k;
            }
            buffer [k - 1] = cur_char;
            cur_char       = get_output ();
        }
        else if (cur_char == u8'e')
        {
            cur_char = u8'E';
        }
    }
    while (cur_char == u8'E' || is_digit (cur_char));

    if (k == config::line_length)
    {
        diagnose.on_fraction_too_long ();
    }

    out_proc.process_fraction ({buffer.data (), k});
    put_back_output (cur_char);
}

// section 121
void
output_token_reducer::send_output_module_number (int module_number)
{
    constexpr size_t buf_size  = config::max_digits + 3;  // digits + 2 braces + 1 colon
    auto             buffer    = std::array<char8_t, buf_size> {};
    auto            *write_ptr = buffer.data ();
    *write_ptr++               = (_brace_level == 0 ? u8'{' : u8'[');
    if (module_number < 0)
    {
        *write_ptr++ = u8':';
        module_number = -module_number;
    }

    ascii_code_t digit_buffer [config::max_digits];
    auto         end   = std::end (digit_buffer);
    auto         begin = to_chars (end, module_number);
    write_ptr          = std::copy (begin, end, write_ptr);

    if (buffer [1] != u8':')
    {
        *write_ptr++ = u8':';
    }

    *write_ptr++ = (_brace_level == 0 ? u8'}' : u8']');
    out_proc.process_string ({buffer.data (), write_ptr});
}

