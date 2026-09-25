#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <print>
#include <string_view>

#include "pascal/array.h"
#include "pascal/range.h"

#include "config.h"

#include "character.h"
#include "change_stream.h"
#include "error.h"
#include "in_stream.h"
#include "name_manager.h"
#include "out_buffer.h"
#include "out_processor.h"
#include "output_token_reducer.h"
#include "output_token_stream.h"
#include "string_pool.h"
#include "tangle.h"
#include "terminal.h"
#include "text_manager.h"
#include "tokens.h"

#include "diagnostics/in_error_manager.h"
#include "diagnostics/in_stream_diagnostics.h"
#include "diagnostics/name_manager_diagnostics.h"
#include "diagnostics/out_buffer_diagnostics.h"
#include "diagnostics/out_error_manager.h"
#include "diagnostics/out_processor_diagnostics.h"
#include "diagnostics/output_token_reducer_diagnostics.h"
#include "diagnostics/output_token_stream_diagnostics.h"
#include "diagnostics/string_pool_diagnostics.h"


static_assert (CHAR_BIT == 8, "Error: This codebase strictly requires an 8-bit char architecture");

using namespace std::literals;
using pascal::operator""_r;

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

in_web_error_manager web_err {err_state};
in_stream_diagnostics web_str_diag {web_err};
in_stream web_str {web_str_diag, config::buf_size - 1};

in_web_error_manager change_err {err_state};
in_stream_diagnostics change_str_diag {change_err};
change_stream change_str {change_str_diag, config::buf_size - 1};

auto *in_str = &web_str;

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

auto double_chars     = size_t {};
auto current_id       = std::u8string_view {};

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

auto str_pool_diag = string_pool_diagnostics {term, web_err};
auto str_pool = string_pool {str_pool_diag};

auto
on_add_string (std::u8string_view id) -> index_t
{
    if (id.length () - double_chars == 2)  // single-character string
        return id [1];

    auto length = id.length () - (double_chars + 1_r);

    return str_pool.add (id, length);
}

auto name_mgr_diag = name_manager_diagnostics {web_err};
auto name_mgr     = name_manager {name_mgr_diag, on_add_string};


// section 65

/// Index in one name
using inname_index_t = pascal::int_range<0, config::longest_name>;
auto mod_text        = pascal::array<inname_index_t, ascii_code_t> {};  /// name being sought for

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

in_stream *
match_target_lines_and_choose_stream ()
{
    if (!web_str.line ().matches (change_str.line ()))
        return &web_str;

    int non_matching_lines = 0;

    while (true)
    {
        if (!change_str.read_line ())
        {
            change_err.err_print ("! Change file ended before @y");
            change_str.line ().clear ();
            return &web_str;
        }

        if (change_str.is_starting_replacement ())
        {
            if (non_matching_lines > 0)
            {
                change_str.seek (2);
                change_str.err ().err_print ("! Hmm... {} of the preceding lines failed to match", 
                    non_matching_lines);
            }

            return &change_str;
        }

        if (!web_str.read_line ())
        {
            web_str.err ().err_print ("! WEB file ended during a change");
            return &web_str;
        }

        if (!web_str.line ().matches (change_str.line ()))
        {
            ++non_matching_lines;
        }
    }
}

// section 133
// section 134
// section 135, 136

bool
get_line ()
{
    while (true)
    {
        if (in_str == &change_str)
        {
            if (change_str.read_replacement_line ())
                break;            

            in_str = &web_str;
        }

        if (!web_str.read_line ())
            break;

        if (change_str.line ().empty ())
            break;
        
        in_str = match_target_lines_and_choose_stream ();
        if (in_str == &web_str)
            break;
    }

    in_str -> seek (0);
    in_str -> line ().pad_end (u8' ');
    return !web_str.eof ();
}

// section 137
// section 138
// section 139

/// control code of no interest to TANGLE
constexpr auto ignore       = ascii_code_t {0};
constexpr auto control_text = ascii_code_t {0x83};  /// control code for ‘@t’, ‘@^’, etc.
constexpr auto format       = ascii_code_t {0x84};  /// control code for ‘@f’
constexpr auto definition   = ascii_code_t {0x85};  /// control code for ‘@d’
constexpr auto begin_pascal = ascii_code_t {0x86};  /// control code for ‘@p’
constexpr auto module_name  = ascii_code_t {0x87};  /// control code for ‘@<’
constexpr auto new_module   = ascii_code_t {0x88};  /// control code for ‘@ ’ and ‘@*’

// Declared in module 171 but needed here already
int module_count;

ascii_code_t
control_code (ascii_code_t c)
{
    switch (c)
    {
    case u8'@'   : return u8'@';
    case u8'\''  : return octal;
    case u8'"'   : return hex;
    case u8'$'   : return check_sum;
    case u8' '   :
    case tab_mark: return new_module;

    case u8'*':
        term.print ("*{}", module_count + 1);
        term.update ();
        return new_module;

    case u8'D':
    case u8'd' : return definition;

    case u8'F' :
    case u8'f' : return format;

    case u8'{' : return begin_comment;
    case u8'}' : return end_comment;

    case u8'P' :
    case u8'p' : return begin_pascal;

    case u8':' :
    case u8'T' :
    case u8't' :
    case u8'^' :
    case u8'.' : return control_text;

    case u8'&' : return join;
    case u8'<' : return module_name;
    case u8'=' : return verbatim;
    case u8'\\': return force_line;

    default    : return ignore;
    }
}

// section 140
// Skips all characters until the next @ or eof
// Returns the control code if one was found
ascii_code_t
skip_ahead ()
{
    while (true)
    {
        if (in_str -> eol ())
        {
            
            if (!get_line ())
                return new_module;
        }

        // Put @ as marker so we don't have to check also for the end
        in_str -> line ().mark_end (u8'@');
        while (in_str -> peek () != u8'@') { in_str -> advance (); }  // find the next marker

        // If we find a @ (other than our own marker) we check the respective control code
        if (!in_str -> eol ())
        {
            in_str -> advance (2);
            auto ascii = in_str -> peek_back ();
            auto c     = control_code (ascii);
            if (c != ignore || ascii == u8'>')
                return c;
        }
    }
}

// section 141, 142

/// Skips to next unmatched '}'
void
skip_comment ()
{
    int balance = 0;
    while (true)
    {
        if (in_str -> eol ())
        {
            if (!get_line ())
            {
                in_str -> err ().err_print ("! Input ended in mid-comment");
                return;
            }
        }

        auto c = in_str -> get();

        if (c == u8'@')
        {
            c = in_str -> peek ();
            if (c == u8' ' || c == tab_mark || c == u8'*')
            {
                in_str -> err ().err_print ("! Section ended in mid-comment");
                in_str -> retreat ();
                return;
            }
            in_str -> advance ();
        }
        else if (c == u8'\\' && in_str -> peek () != u8'@')
        {
            in_str -> advance ();
        }
        else if (c == u8'{')
        {
            ++balance;
        }
        else if (c == u8'}')
        {
            if (balance == 0)
                return;

            --balance;
        }
    }
}

// section 143, 144

/// name of module just scanned
name_t * cur_module_name;
bool    scanning_hex = false;  /// are we scanning a hexadecimal constant

// section 145 - 155
ascii_code_t
get_identifier (ascii_code_t c);
ascii_code_t
get_preprocessed_string ();
void
scan_module_name ();

inline bool
compress_if (char8_t &c, char8_t match, char8_t compressed)
{
    if (in_str -> peek () != match)
        return false;

    if (!in_str -> eol ())
    {
        c = compressed;
        in_str -> advance ();
    }

    return true;
}

uint8_t
get_next ()
{
    while (true)
    {
        if (in_str -> eol ())
        {
            if (!get_line ())
                return new_module;
        }
        auto c = in_str -> get ();

        if (scanning_hex)
        {
            if (is_hex (c))
                return c;

            scanning_hex = false;
        }

        if (is_alpha (c))
            return get_identifier (c);

        switch (c)
        {
        case u8'"': return get_preprocessed_string ();

        case u8'@':
            c = control_code (in_str -> get ());
            if (c == ignore)
                continue;

            if (c == hex)
            {
                scanning_hex = true;
            }
            else if (c == module_name)
            {
                scan_module_name ();
            }
            else if (c == control_text)
            {
                do { c = skip_ahead (); }
                while (c == u8'@');

                if (in_str -> peek_back () != u8'>')
                {
                    in_str -> err ().err_print ("! Improper @ within control text");
                }

                continue;
            }
            return c;

        // section 147
        case u8'.': compress_if (c, '.', double_dot) || compress_if (c, ')', u8']'); return c;
        case u8':': compress_if (c, '=', left_arrow); return c;
        case u8'=': compress_if (c, '=', equivalence_sign); return c;
        case u8'>': compress_if (c, '=', greater_or_equal); return c;
        case u8'<': compress_if (c, '=', less_or_equal) || compress_if (c, '>', not_equal); return c;
        case u8'(': compress_if (c, '*', begin_comment) || compress_if (c, '.', u8'['); return c;
        case u8'*': compress_if (c, ')', end_comment); return c;
        case u8' ':
        case tab_mark: continue;
        case u8'{': skip_comment (); continue;
        case u8'}': in_str -> err ().err_print ("! Extra }}"); continue;

        default:
            if (c >= 128)
                continue;

            return c;
        }
    }
}

// section 148
ascii_code_t
get_identifier (ascii_code_t c)
{
    if (in_str -> tell () > 1
        && (c == u8'E' || c == u8'e')
        && is_digit (in_str -> peek_back (2)) // the char before c
    )  
        return u8'E';

    ascii_code_t d;
    auto id_first = in_str -> tell () - 1;
    do
    {
        d = in_str -> get ();
    }
    while (is_alphanumeric (d) || d == u8'_');
    in_str -> retreat ();

    if (in_str -> tell () > id_first + 1)
    {
        c          = identifier;
        current_id = {& in_str -> line () [id_first], static_cast<size_t> (in_str -> tell () - id_first)};
    }

    return c;
}

// section 149
ascii_code_t
get_preprocessed_string ()
{
    ascii_code_t d;
    double_chars  = 0_r;
    auto id_first = in_str -> tell () - 1_r;

    do
    {
        d = in_str -> get ();
        if (d == u8'"' || d == u8'@')
        {
            if (in_str -> peek () == d)
            {
                in_str -> advance ();
                d = 0;
                ++double_chars;
            }
            else if (d == u8'@')
            {
                in_str -> err ().err_print ("! Double @ sign missing");
            }
        }
        else if (in_str -> eol ())
        {
            in_str -> err ().err_print ("! String constant didn't end");
            d = u8'"';
        }
    }
    while (d != u8'"');

    current_id = {& in_str -> line () [id_first], static_cast<size_t> (in_str -> tell () - 1 - id_first)};
    return identifier;
}

// section 151

/// Puts module name in mod_text[1..length]
/// Returns the length
auto
put_module_name_in_mod_text () -> inname_index_t;

void
scan_module_name ()
{
    auto k = put_module_name_in_mod_text ();

    if (k > 3)
    {
        if (mod_text [k] == u8'.' && mod_text [k - 1_r] == u8'.' && mod_text [k - 2_r] == u8'.')
        {
            cur_module_name = &name_mgr.lookup_prefix ({&mod_text[1_r], static_cast <size_t> (k) - 3});
        }
        else
        {
            cur_module_name = &name_mgr.lookup_module ({&mod_text [1_r], k});
        }
    }
}

// section 152 nothing tbd

// section 153
auto
put_module_name_in_mod_text () -> inname_index_t
{
    auto d = ascii_code_t {0};
    auto k = inname_index_t {0};
    while (true)
    {
        if (in_str -> eol ())
        {
            if (!get_line ())
            {
                in_str -> err ().err_print ("! Input has ended in section name");
                break;
            }
        }

        d = in_str -> peek ();
        if (d == u8'@')
        {
            d = in_str -> peek_ahead ();
            if (d == u8'>')
            {
                in_str -> advance (2);
                break;
            }
            if (d == u8' ' || d == tab_mark || d == u8'*')
            {
                in_str -> err ().err_print ("! Section name didn't end");
                break;
            }
            ++k;
            mod_text [k] = u8'@';
            in_str -> advance ();
        }

        in_str -> advance ();
        if (k < config::longest_name - 1)
        {
            ++k;
        }
        if (d == u8' ' || d == tab_mark)
        {
            d = u8' ';
            if (mod_text [k - 1_r] == u8' ')
            {
                --k;
            }
        }
        mod_text [k] = d;
    }

    if (k > config::longest_name - 2)
    {
        in_str -> err ().terminal ().print_nl ("! Section name too long: ");
        in_str -> err ().terminal ().print ({&mod_text.data ()[1], 25});
        in_str -> err ().terminal ().print ("...");
        in_str -> err ().mark_harmless ();
    }

    if (k > 0 && mod_text [k] == u8' ')
    {
        --k;
    }

    return k;
}

// section 156

bool
end_of_definition (ascii_code_t c)
{ return c >= format; }

ascii_code_t next_control;

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
            next_control = get_next ();
        }
        while (is_digit (next_control));

        accumulator += next_sign * val;
        next_sign = 1_r;
        return scan_numeric_cases::reswitch;
    }

    switch (next_control)
    {
    case octal:
        next_control = u8'0';
        do
        {
            val          = 8 * val + next_control - u8'0';
            next_control = get_next ();
        }
        while (is_octal (next_control));

        accumulator += next_sign * val;
        next_sign = 1_r;
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
            next_control = get_next ();
        }
        while (is_hex (next_control));

        accumulator += next_sign * val;
        next_sign = 1_r;
        return scan_numeric_cases::reswitch;

    case identifier:
    {
        auto &name = name_mgr.lookup (normal, current_id);
        if (name.ilk () != numeric)
        {
            next_control = u8'*';  // leads to error
            return scan_numeric_cases::reswitch;
        }

        accumulator += next_sign * name.number();
        next_sign = 1_r;
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
        in_str -> err ().err_print ("! Omit semicolon in numeric definition");
        return scan_numeric_cases::consumed;

    default:
        in_str -> err ().err_print ("! Improper numeric definition will be flushed");
        do { next_control = skip_ahead (); }
        while (!end_of_definition (next_control));

        if (next_control == module_name)  // we want to scan the module name too
        {
            in_str -> retreat (2);
            next_control = get_next ();
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
        next_control = get_next ();
        do { state = scan_numeric_one (accumulator, next_sign); }
        while (state == scan_numeric_cases::reswitch);
    }
    while (state != scan_numeric_cases::done);

    if (std::abs (accumulator) >= 0x8000)
    {
        in_str -> err ().err_print ("! Value too big: ", accumulator);
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
    index_t a;

    bool    done = false;

    do
    {
        a = get_next ();

        switch (a)
        {
        case u8'(': 
            ++balance; 
            text_mgr.append_to_next_new (a);
            break;

        case u8')':
            if (balance == 0)
            {
                in_str -> err ().err_print ("! Extra )");
            }
            else
            {
                --balance;
            }
            text_mgr.append_to_next_new (a);
            break;

        case u8'\'': 
            copy_string_from_buffer_to_text_mgr (); 
            break;

        case u8'#':
            if (type == parametric)
            {
                a = param;
            }
            text_mgr.append_to_next_new (a);
            break;

        case identifier:
        {
            auto &name = name_mgr.lookup (normal, current_id);
            text_mgr.append_to_next_new (0x8000 + name_mgr.index_of (name));
            break;
        }

        case module_name:
            if (type == module_name)
            {
                text_mgr.append_to_next_new (0xA800 + name_mgr.index_of (*cur_module_name));
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
                in_str -> err ().err_print (
                    "! @ {} is ignored in Pascal text",
                    convert_to_output (in_str -> peek_back ()));
                continue;
            }
            done = true;
            break;

        case new_module: done = true; break;
        
        default:
            text_mgr.append_to_next_new (a);
            break;
        }
    }
    while (!done);

    next_control = a & 0xFF;
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
            in_str -> err ().err_print ("! Missing )");
        }
        else
        {
            in_str -> err ().err_print ("! Missing {} )'s", balance);
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
            if (in_str -> peek () == u8'@')
            {
                in_str -> advance ();  // store only one @
            }
            else
            {
                in_str -> err ().err_print ("! You should double @ signs in strings");
            }
        }

        if (in_str -> end_of_content ())
        {
            in_str -> err ().err_print ("! String didn't end");
            in_str -> patch (u8"'\0");
        }

        b = in_str -> get ();
        if (b == u8'\'')
        {
            if (in_str -> peek () != u8'\'')
                break;

            in_str -> advance ();
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
    in_str -> line ().mark_end (u8'@');

    while (true)
    {
        if (in_str -> peek () == u8'@')
        {
            if (!in_str -> end_of_content ())
            {
                if (in_str -> peek_ahead () == u8'@')
                {
                    text_mgr.append_to_next_new (U'@');
                    in_str -> advance (2);
                    continue;
                }
            }
        }
        else
        {
            text_mgr.append_to_next_new (static_cast<text_manager::char_type> (in_str -> get ()));
            continue;
        }

        break;
    }

    if (in_str -> end_of_content ())
    {
        in_str -> err ().err_print ("! Verbatim string didn't end");
    }
    else if (in_str -> peek_ahead () != u8'>')
    {
        in_str -> err ().err_print ("! You shouldn't double @ signs in verbatim strings");
    }

    in_str -> advance (2);
    text_mgr.append_to_next_new (verbatim);
}

// section 170

void
define_macro (ilk_value type)
{
    auto &name = name_mgr.lookup (type, current_id);
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
            next_control = skip_ahead ();
            if (next_control == module_name)  // we want to scan the module name too
            {
                in_str -> retreat (2);
                next_control = get_next ();
            }
        }

        if (next_control != definition)
            return;

        next_control = get_next ();  // get identifier name
        if (next_control != identifier)
        {
            in_str -> err ().err_print ("! Definition flushed must start with identifier of length > 1");
            continue;
        }
        next_control = get_next ();  // get token after the identifier

        if (next_control == u8'=')
        {
            name_mgr.lookup (numeric, current_id).set_number (scan_numeric());
            continue;
        }

        if (next_control == equivalence_sign)
        {
            define_macro (simple);
            continue;
        }

        if (next_control == u8'(')
        {
            next_control = get_next ();
            if (next_control == u8'#')
            {
                next_control = get_next ();
                if (next_control == u8')')
                {
                    next_control = get_next ();
                    if (next_control == u8'=')
                    {
                        in_str -> err ().err_print ("! Use == for macros");
                        next_control = equivalence_sign;
                    }
                    if (next_control == equivalence_sign)
                    {
                        define_macro (parametric);
                        continue;
                    }
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
        scanned_module_name = cur_module_name;

        do { next_control = get_next (); }
        while (next_control == u8'+');

        if (next_control != u8'=' && next_control != equivalence_sign)
        {
            in_str -> err ().err_print ("! Pascal text flushed, = sign is missing");
            do { next_control = skip_ahead (); }
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
    web_err.set_stream(&web_str);
    change_err.set_stream(&change_str);

    web_str.open (web_file_name);
    change_str.open (change_file_name);

    str_pool.initialize (pool_file_name);
    name_mgr.initialize (config::max_bytes, config::max_names);
    text_mgr.initialize (config::max_toks, config::max_texts);

    term.print_ln ("{}", config::banner);

    in_str = &web_str;
    scanning_hex = false;
    mod_text [0_r] = u8' ';
    module_count = 0;

    change_str.read_next_target_line ();
    do { next_control = skip_ahead (); }
    while (next_control != new_module);

    while (!web_str.eof ()) { scan_module (); }
    change_str.check_if_processed_all_changes ();
    web_str.close ();
    change_str.close ();

    out_buf.initialize (pascal_file_name);
    out_err.set_buffer (&out_buf);
    output_compressed_tables (term);
    str_pool.finalize ();
    out_buf.finalize ();

    return in_str -> err ().exit_code ();
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
