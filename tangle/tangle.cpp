#include <climits>
#include <cstdint>
#include <print>
#include <string_view>

#include "config.h"

#include "character.h"
#include "error.h"
#include "input_token_stream.h"
#include "name.h"
#include "name_manager.h"
#include "out_buffer.h"
#include "out_processor.h"
#include "output_token_reducer.h"
#include "output_token_stream.h"
#include "patched_in_stream.h"
#include "string_pool.h"
#include "tangle.h"
#include "terminal.h"
#include "text_manager.h"
#include "tokens.h"

#include "diagnostics/change_stream_diagnostics.h"
#include "diagnostics/in_error_manager.h"
#include "diagnostics/in_stream_diagnostics.h"
#include "diagnostics/input_token_stream_diagnostics.h"
#include "diagnostics/name_manager_diagnostics.h"
#include "diagnostics/out_buffer_diagnostics.h"
#include "diagnostics/out_error_manager.h"
#include "diagnostics/out_processor_diagnostics.h"
#include "diagnostics/output_token_reducer_diagnostics.h"
#include "diagnostics/output_token_stream_diagnostics.h"
#include "diagnostics/patched_in_stream_diagnostics.h"
#include "diagnostics/string_pool_diagnostics.h"


static_assert (CHAR_BIT == 8, "Error: This codebase strictly requires an 8-bit char architecture");

using namespace std::literals;

// section 1
// section 2 - 7 n/a
// initialize will be defined in section 182 just before main because no variables have been declared,
// yet
// section 8
// section 9
// section 10
// section 11
// section 12
// For text_file we use pascal::text_file.
// section 13, 14, 16, 17
// section 15
// section 18
// section 19 nothing tbd
// section 20
// section 21 nothing tbd
// section 22
// section 23
terminal          term {stdout};
error_state       err_state {term};

// section 24
// section 25
// section 26
// section 27
// section 28
// section 29
// section 30
// section 31
// section 32
// section 33
out_error_manager out_err {err_state};

auto out_buf_diag = out_buffer_diagnostics {term, out_err};
auto out_buf = out_buffer {config::line_length, out_buf_diag};

// section 34
// section 35
// section 36
// section 37
// section 38

using index_t = config::index_t;  /// used to store indices in arrays

text_manager text_mgr;

// section 39
// section 40
// section 41, 42, 43 not required (global arrays are zero-initialized in C++),
// other initializers already given in section 40

// section 44
// section 45, 46 not required
// section 47
// section 48
// section 49
// section 50
// section 51, 52 not required
// section 53
// section 54
// section 55
// section 56
// section 57
// section 58
// section 59
// section 60
// section 61
// section 62
// section 63
// section 64

in_web_error_manager web_err {err_state};
in_web_error_manager change_err {err_state};

auto str_pool_diag = string_pool_diagnostics {term, web_err};
auto str_pool = string_pool {str_pool_diag};

auto name_mgr_diag = name_manager_diagnostics {web_err};
auto name_mgr     = name_manager {name_mgr_diag, str_pool};

// section 65
// section 66
// section 67
// section 68
// section 69
// section 70
// section 71 not required

// section 73
// section 74, 75, 76 implement print_repl for debug mode if that gets included
// section 77 nothing tbd
// section 78


auto output_token_str_diag = output_token_stream_diagnostics {out_err};
auto output_token_str      = output_token_stream {name_mgr, text_mgr, output_token_str_diag};
auto out_proc_diag         = out_processor_diagnostics {out_err};
auto out_proc              = out_processor {out_buf, out_proc_diag};
auto output_token_red_diag = output_token_reducer_diagnostics {out_err};
auto output_token_red      = output_token_reducer {
    output_token_str, out_proc, str_pool.check_sum(), output_token_red_diag};

/// section 80
// section 81 nothing tbd
// section 82
// section 83
// section 84
// section 85
// section 86
// section 87, 88, 89, 90, 92
// section 91
// section 93 .
// section 94
// section 95
// section 96
// section 97
// section 98
// section 99
// section 100
// section 101
// section 102
// section 103
// section 104, 105
// section 106
// section 107, 108
// section 109
// section 110
// section 111
// section 112

void
output_compressed_tables (terminal &term)
{
    if (!text_mgr.root ().continuation ())
    {
        out_err.terminal ().print_nl ("! No output was specified.");
        out_err.mark_harmless ();
        return;
    }

    term.print_nl ("Writing the output file");
    term.update ();

    output_token_str.initialize();
    output_token_red.initialize ();
    output_token_red.send_the_output ();
    out_buf.flush_last_line ();

    auto brace_level = output_token_red.brace_level ();
    if (brace_level != 0)
    {
        out_err.err_print ("! Program ended at brace level {}", brace_level);
    }
    term.print_nl ("Done.");
}

// section 122
// section 123 nothing tbd

// section 124
// Section 125
// section 126
// section 127
// section 128
// section 129
// section 130
// section 131
// section 132
// section 133
// section 134
// section 135, 136

patched_in_stream_diagnostics patched_in_diag {web_err, change_err};
in_stream_diagnostics web_str_diag {web_err};
change_stream_diagnostics change_str_diag {change_err};

patched_in_stream in_str {patched_in_diag, web_str_diag, change_str_diag, config::buf_size - 1};

// section 137
// section 138
// section 139

// Declared in module 171 but needed here already
size_t module_count;

// section 140
// section 141, 142
// section 143, 144
// section 145 - 155

// section 148
// section 149
// section 151
// section 156

bool
end_of_definition (ascii_code_t c)
{ return c >= format; }

auto in_tok_str_diag = input_token_stream_diagnostics {term, module_count};
auto in_tok_str = input_token_stream {in_tok_str_diag, in_str, name_mgr};

char8_t next_control = 0;

char8_t
get_next ()
{
    return next_control = in_tok_str.get ();
}

// section 157, 158, 159, 160, 161, 162

enum class scan_numeric_cases
{
    consumed,
    done,
    reswitch,
};

auto
scan_numeric_one (int &accumulator, int &next_sign) -> scan_numeric_cases
{
    int     val = 0;
    index_t q;

    if (is_digit (next_control))
    {
        do
        {
            val          = 10 * val + next_control - u8'0';
        }
        while (is_digit (get_next ()));

        accumulator += next_sign * val;
        next_sign = 1;
        return scan_numeric_cases::reswitch;
    }

    switch (next_control)
    {
    case octal:
        next_control = u8'0';
        do
        {
            val          = 8 * val + next_control - u8'0';
        }
        while (is_octal (get_next ()));

        accumulator += next_sign * val;
        next_sign = 1;
        return scan_numeric_cases::reswitch;

    case hex:
        next_control = u8'0';
        do
        {
            if (next_control >= 'A')
            {
                next_control += u8'0' - (u8'A' - 10);
            }
            val          = 16 * val + next_control - u8'0';
        }
        while (is_hex (get_next ()));

        accumulator += next_sign * val;
        next_sign = 1;
        return scan_numeric_cases::reswitch;

    case identifier:
    {
        auto &name = in_tok_str.current_identifier (normal);
        if (name.ilk () != numeric)
        {
            next_control = u8'*';  // leads to error
            return scan_numeric_cases::reswitch;
        }

        accumulator += next_sign * name.number();
        next_sign = 1;
        return scan_numeric_cases::consumed;
    }

    case u8'+'       : return scan_numeric_cases::consumed;

    case u8'-'       : next_sign = -next_sign; return scan_numeric_cases::consumed;

    case format      :
    case definition  :
    case module_name :
    case begin_pascal:
    case new_module  : return scan_numeric_cases::done;

    case u8';':
        in_str.err ().err_print ("! Omit semicolon in numeric definition");
        return scan_numeric_cases::consumed;

    default:
        in_str.err ().err_print ("! Improper numeric definition will be flushed");
        do { next_control = in_tok_str.get_next_control_code (); }
        while (!end_of_definition (next_control));

        if (next_control == module_name)  // we want to scan the module name too
        {
            in_str.retreat (2);
            get_next ();
        }

        accumulator = 0;
        return scan_numeric_cases::done;
    }
}

/// defines numeric macros
int32_t
scan_numeric ()
{
    int                accumulator = 0;  /// accumulates sums
    int                next_sign   = 1;  /// sign to attach to next value

    scan_numeric_cases state;
    do
    {
        get_next ();
        do { state = scan_numeric_one (accumulator, next_sign); }
        while (state == scan_numeric_cases::reswitch);
    }
    while (state != scan_numeric_cases::done);

    if (std::abs (accumulator) >= 0x8000)
    {
        in_str.err ().err_print ("! Value too big: ", accumulator);
        accumulator = 0;
    }
    return accumulator;
}

// section 163 nothing tbd
// section 164
// section 165, 167

void
copy_string_from_buffer_to_text_mgr ();
void
copy_verbatim_from_buffer_to_text_mgr ();
void
ensure_parantheses_balance (int &balance);

auto &
scan_replacement (uint8_t type)
{
    int     balance = 0;  /// left parentheses minus right parentheses
    bool    done = false;

    do
    {
        auto ch = get_next ();
        switch (ch)
        {
        case u8'(': 
            ++balance; 
            text_mgr.append_to_next_new (static_cast <text_manager::char_type> (ch));
            break;

        case u8')':
            if (balance == 0)
            {
                in_str.err ().err_print ("! Extra )");
            }
            else
            {
                --balance;
            }
            text_mgr.append_to_next_new (static_cast <text_manager::char_type> (ch));
            break;

        case u8'\'': 
            copy_string_from_buffer_to_text_mgr (); 
            break;

        case u8'#':
            text_mgr.append_to_next_new (static_cast <text_manager::char_type> (
                type == parametric ? param: ch));
            break;

        case identifier:
        {
            auto &name = in_tok_str.current_identifier (normal);
            text_mgr.append_to_next_new (0x8000 + name_mgr.index_of (name));
            break;
        }

        case module_name:
            if (type == module_name)
            {
                auto index = name_mgr.index_of (in_tok_str.current_module_name ());
                text_mgr.append_to_next_new (0xA800 + index);
                break;
            }
            done = true;
            break;

        case verbatim: 
            copy_verbatim_from_buffer_to_text_mgr (); 
            break;

        case definition:
        case format:
        case begin_pascal:
            if (type == module_name)
            {
                in_str.err ().err_print (
                    "! @ {} is ignored in Pascal text",
                    convert_to_output (in_str.peek_back ()));
                continue;
            }
            done = true;
            break;

        case new_module: done = true; break;
        
        default:
            text_mgr.append_to_next_new (static_cast <text_manager::char_type> (ch));
            break;
        }
    }
    while (!done);

    ensure_parantheses_balance (balance);

    return text_mgr.add_next_new ();
}

// section 166

void
ensure_parantheses_balance (int &balance)
{
    if (balance > 0)
    {
        if (balance == 1)
        {
            in_str.err ().err_print ("! Missing )");
        }
        else
        {
            in_str.err ().err_print ("! Missing {} )'s", balance);
        }
    }

    while (balance > 0)
    {
        text_mgr.append_to_next_new (U')');
        --balance;
    }
}

// section 168

void
copy_string_from_buffer_to_text_mgr ()
{
    char8_t b = u8'\'';

    while (true)
    {
        text_mgr.append_to_next_new (static_cast<text_manager::char_type>(b));
        if (b == u8'@')
        {
            if (in_str.peek () == u8'@')
            {
                in_str.advance ();  // store only one @
            }
            else
            {
                in_str.err ().err_print ("! You should double @ signs in strings");
            }
        }

        if (in_str.end_of_content ())
        {
            in_str.err ().err_print ("! String didn't end");
            in_str.patch (u8"'\0");
        }

        b = in_str.get ();
        if (b == u8'\'')
        {
            if (in_str.peek () != u8'\'')
                break;

            in_str.advance ();
            text_mgr.append_to_next_new (u8'\'');
        }
    }
    text_mgr.append_to_next_new (u8'\'');
}

// section 169

void
copy_verbatim_from_buffer_to_text_mgr ()
{
    text_mgr.append_to_next_new (verbatim);
    in_str.line ().mark_end (u8'@');

    while (true)
    {
        if (in_str.peek () == u8'@')
        {
            if (!in_str.end_of_content () && in_str.peek_ahead () == u8'@')
            {
                    text_mgr.append_to_next_new (U'@');
                    in_str.advance (2);
                    continue;
            }
        }
        else
        {
            text_mgr.append_to_next_new (static_cast<text_manager::char_type> (in_str.get ()));
            continue;
        }

        break;
    }

    if (in_str.end_of_content ())
    {
        in_str.err ().err_print ("! Verbatim string didn't end");
    }
    else if (in_str.peek_ahead () != u8'>')
    {
        in_str.err ().err_print ("! You shouldn't double @ signs in verbatim strings");
    }

    in_str.advance (2);
    text_mgr.append_to_next_new (verbatim);
}

// section 170

void
define_macro (ilk_value type)
{
    auto &name = in_tok_str.current_identifier (type);
    auto &replacement_text = scan_replacement (type);
    name.set_replacement_text (replacement_text);
    replacement_text.set_continuation (&text_mgr.root());
}

// section 171 nothing tbd

// section 172
void
scan_definition_part ();
void
scan_pascal_part ();

void
scan_module ()
{
    ++module_count;
    scan_definition_part ();
    scan_pascal_part ();
}

// section 173, 174

void
scan_definition_part ()
{
    next_control = 0;

    while (true)
    {
        while (next_control <= format)
        {
            next_control = in_tok_str.get_next_control_code ();;
            if (next_control == module_name)  // we want to scan the module name too
            {
                in_str.retreat (2);
                get_next ();
            }
        }

        if (next_control != definition)
            return;

        if (get_next () != identifier)
        {
            in_str.err ().err_print ("! Definition flushed must start with identifier of length > 1");
            continue;
        }
 
        switch (get_next ())
        {
        case u8'=':
            in_tok_str.current_identifier (numeric).set_number (scan_numeric());
            continue;

        case equivalence_sign:
            define_macro (simple);
            continue;

        case u8'(':
            if (get_next () == u8'#'
             && get_next () == u8')')
            {
                switch (get_next ())
                {
                case u8'=':
                    in_str.err ().err_print ("! Use == for macros");
                    [[fallthrough]];
                case equivalence_sign:
                    define_macro (parametric);
                    continue;
                }
            }
        }
    }
}

// section 175, 176

void
scan_pascal_part ()
{
    name_t * scanned_module_name = &name_mgr.no_name();
    switch (next_control)
    {
    case begin_pascal: break;
    case module_name:
        scanned_module_name = &in_tok_str.current_module_name ();

        while (get_next () == u8'+') {}

        if (next_control != u8'=' && next_control != equivalence_sign)
        {
            in_str.err ().err_print ("! Pascal text flushed, = sign is missing");
            do { next_control = in_tok_str.get_next_control_code (); }
            while (next_control != new_module);
            return;
        }
        break;

    default: 
        return;
    }

    text_mgr.append_to_next_new (0xD000 + module_count);
    auto &replacement_text = scan_replacement (module_name);
    replacement_text.set_continuation (nullptr);  // mark this replacement text as nonmacro

    if (scanned_module_name == &name_mgr.no_name ())
    {
        text_mgr.add_unnamed (replacement_text);
    }
    else 
    {
        scanned_module_name -> add_replacement_text (replacement_text);
    }
}

// section 179, 180, 181: debugging, left out for now

// section 182
int
tangle (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name)
{
    in_str.open (web_file_name, change_file_name);

    str_pool.initialize (pool_file_name);
    name_mgr.initialize (config::max_bytes, config::max_names);
    text_mgr.initialize (config::max_toks, config::max_texts);

    term.print_ln ("{}", config::banner);

    in_tok_str.initialize();
    module_count = 0;

    do { next_control = in_tok_str.get_next_control_code (); }
    while (next_control != new_module);

    while (!in_str.eof ()) { scan_module (); }

    in_str.close ();

    out_buf.initialize (pascal_file_name);
    out_err.set_buffer (&out_buf);
    output_compressed_tables (term);
    str_pool.finalize ();
    out_buf.finalize ();

    return err_state.exit_code ();
}

int
tangle_exceptions_handled (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name)
{
    try
    {
        return tangle (web_file_name, change_file_name, pascal_file_name, pool_file_name);
    }
    catch (const std::filesystem::filesystem_error &ex)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] File system exception!");
        std::println (stderr, "What: {}", ex.what ());
        std::println (stderr, "Path 1: {}", ex.path1 ().string ());
        return 4;
    }
    catch (const std::exception &ex)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] Standard Exception Caught!");
        std::println (stderr, "What: {}", ex.what ());
        return 5;
    }
    catch (...)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] An unknown non-standard error occurred!");
        return 6;
    }
}
