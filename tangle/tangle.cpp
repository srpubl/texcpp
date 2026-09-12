#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <print>
#include <string_view>
#include <vector>

#include "diagnostics/output_token_reducer_diagnostics.h"
#include "output_token_reducer.h"
#include "utility/between.h"

#include "pascal/array.h"
#include "pascal/range.h"
#include "pascal/text_file.h"

#include "config.h"

#include "character.h"
#include "error.h"
#include "name_manager.h"
#include "out_buffer.h"
#include "out_processor.h"
#include "output_token_stream.h"
#include "tangle.h"
#include "terminal.h"
#include "text_manager.h"
#include "tokens.h"

#include "diagnostics/name_manager_diagnostics.h"
#include "diagnostics/out_processor_diagnostics.h"
#include "diagnostics/out_buffer_diagnostics.h"
#include "diagnostics/output_token_stream_diagnostics.h"


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
pascal::text_file web_file;
pascal::text_file change_file;

terminal          term {stdout};
error_manager     err {term};

// section 24
void
open_input ()
{
    web_file.reset ();
    change_file.reset ();
}

// section 25
pascal::text_file pascal_file;
pascal::text_file pool;

// section 26
void
open_output ()
{
    pascal_file.rewrite ();
    pool.rewrite ();
}

// section 27
using buf_index_t = pascal::int_range<0, config::buf_size>;
auto buffer       = pascal::array<buf_index_t, ascii_code_t> {};  /// The input line buffer. Holds valid
                                                                  /// content from 0 to limit-1.

// section 28

// Declared in section 124 but we need them here already
/// the last character position occupied in the buffer
auto limit = buf_index_t {};
auto loc   = buf_index_t {};  /// the next character position to be read from the buffer

// Reads a line from the given file into the buffer array, converting characters to their ASCII codes.
// Returns true if a line was read, false if the end of the file was reached.
bool
input_ln (pascal::text_file &file)
{
    auto final_limit = buf_index_t {};  /// limit without trailing blanks
    limit            = 0_r;

    if (file.eof ())
        return false;

    while (!file.eol ())
    {
        buffer [limit] = convert_from_input (file.current ());
        file.get ();
        ++limit;

        if (buffer [limit - 1_r] != u8' ')
        {
            final_limit = limit;
        }

        // If input line is longer than buffer: discard all extra characters and signal error
        if (limit == config::buf_size)
        {
            while (!file.eol ()) { file.get (); }
            --limit;
            if (final_limit > limit)
            {
                final_limit = limit;
            }
            loc = 0_r;
            err.err_print ("! Input line too long");
        }
    }

    file.read_line ();
    limit = final_limit;
    return true;
}

// section 29
// section 30 nothing tbd
// section 31
// section 32

///  if true, the current line is from change file
bool changing = true;
int  line     = 0;  /// the number of the current line in the current file

void
print_error_location_input (terminal &term)
{
    if (changing)
    {
        term.print (". (change file ");
    }
    else
    {
        term.print (". (");
    }
    term.print_ln ("l.{})", line);

    // print characters already read
    auto l = std::min (loc, limit);
    for (buf_index_t k = 0_r; k < l; ++k)
    {
        auto ch = buffer [k];
        term.print (ch == tab_mark ? u8' ' : ch);
    }
    term.print_nl ("{:>{}}", "", int {l});

    // print not yet read characters
    term.print ({&buffer[l], &buffer[limit]});
    term.print (' ');
}

// section 33

auto out_buf_diag = out_buffer_diagnostics {term, err};
auto out_buf = out_buffer {config::line_length, pascal_file, out_buf_diag};

void
print_error_location_output (terminal &term)
{
    term.print_ln (". (l.{})", out_buf.current_line ());
    term.print (out_buf.temporary_view ());
    term.print ("... ");
}

// section 34
// section 35
// section 36
// section 37
// We use uint8_t and uint16_t instead of eight_bits and sixteen_bits
// section 38

using index_t         = config::index_t;  /// used to store indices in arrays

text_manager text_mgr;

// section 39
// section 40

auto string_ptr     = index_t {256};  /// next number to be given to a string of length > 1
int  pool_check_sum = 271828;         /// sort of a hash for the whole string pool

// section 41, 42, 43 not required (global arrays are zero-initialized in C++),
// other initializers already given in section 40

// section 44
// section 45, 46 not required
// section 47
// section 48
// section 49
// section 50

auto double_chars     = buf_index_t {};
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

constexpr auto checksum_prime = (1 << 29) - 73;

void
add_to_checksum (int value)
{
    pool_check_sum += pool_check_sum + value;
    while (pool_check_sum > checksum_prime) { pool_check_sum -= checksum_prime; }
}

void
add_string_to_pool (std::u8string_view id, size_t actual_length)
{
    // output length
    write (pool, u8'0' + actual_length / 10);
    write (pool, u8'0' + actual_length % 10);

    add_to_checksum (actual_length);

    bool skip_one = true;  // skip first element and every doubled " or @
    for (auto ch : id)
    {
        if (skip_one)
        {
            skip_one = false;
            continue;
        }
        write (pool, ch);
        add_to_checksum (ch);
        if (ch == u8'"' || ch == u8'@')
        {
            skip_one = true;
        }
    }
    pool.write_line ();
}

auto
on_add_string (std::u8string_view id) -> index_t
{
    if (id.length () - double_chars == 2)  // single-character string
        return id [1];

    auto length = id.length () - (double_chars + 1_r);
    if (length > 99)
    {
        err.err_print ("! Preprocessed string is too long");
    }

    add_string_to_pool (id, length);

    return string_ptr++;
}

auto name_mgr_diag = name_manager_diagnostics {err};
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

text_t *last_unnamed = &text_mgr.storage.record_0();  /// most recent replacement text of unnamed module

// section 71 not required

// section 73
// section 74, 75, 76 implement print_repl for debug mode if that gets included
// section 77 nothing tbd
// section 78


auto output_token_str_diag = output_token_stream_diagnostics {err};
auto output_token_str      = output_token_stream {name_mgr, text_mgr, output_token_str_diag};
auto out_proc_diag         = out_processor_diagnostics {err};
auto out_proc              = out_processor {out_buf, out_proc_diag};
auto output_token_red_diag = output_token_reducer_diagnostics {err};
auto output_token_red      = output_token_reducer {output_token_str, out_proc, pool_check_sum, output_token_red_diag};

/// section 80
// section 81 nothing tbd
// section 82
// section 83

void
initialize_output_stacks ()
{
    output_token_str.initialize();
    output_token_red.initialize ();
}

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
    if (!text_mgr.storage.record_0().continuation())
    {
        err.terminal ().print_nl ("! No output was specified.");
        err.mark_harmless ();
        return;
    }

    term.print_nl ("Writing the output file");
    term.update ();
    initialize_output_stacks ();
    output_token_red.send_the_output ();
    out_buf.flush_last_line ();
    auto brace_level = output_token_red.brace_level ();
    if (brace_level != 0)
    {
        err.err_print ("! Program ended at brace level {}", brace_level);
    }
    term.print_nl ("Done.");
}

// section 122
// section 123 nothing tbd

// section 124

int ii;
int other_line = 0;
// int temp_line; // not needed because we use std::swap
bool input_has_ended;

// line, limit, loc, changing had to be declared earlier

// Section 125

void
change_changing ()
{
    changing = !changing;
    std::swap (line, other_line);
}

// section 126

auto change_buffer = pascal::array<buf_index_t, ascii_code_t> {};
auto change_limit  = buf_index_t {};

// section 127

auto
lines_dont_match () -> bool
{
    if (limit != change_limit)
        return true;

    for (auto k = buf_index_t {0}; k < limit; ++k)
    {
        if (buffer [k] != change_buffer [k])
            return true;
    }
    return false;
}

// section 128

bool
skip_to_start_of_change ();
bool
skip_blank_lines ();
void
copy_line_to_change_buffer ();

void
prime_the_change_buffer ()
{
    change_limit = 0_r;

    if (!skip_to_start_of_change ())
        return;

    if (!skip_blank_lines ())
        return;

    copy_line_to_change_buffer ();
}

// section 129

ascii_code_t
get_change_control_letter ()
{
    auto &c = buffer [1_r];

    if (is_between (c, u8'X', u8'Z'))
    {
        c += (u8'z' - u8'Z');
    }

    return c;
}

/// searches for an @x or @X at beginning of a line in the change file, and reports an error if it finds
/// @y or @z before that. Returns true if @x was found
bool
skip_to_start_of_change ()
{
    while (true)
    {
        ++line;
        if (!input_ln (change_file))
            return false;

        if (limit < 2 || buffer [0_r] != u8'@')
            continue;

        switch (get_change_control_letter ())
        {
        case u8'x': return true;
        case u8'y':
        case u8'z': loc = 2_r; err.err_print ("! Where is the matching @x?");
        }
    }
}

// section 130
bool
skip_blank_lines ()
{
    do
    {
        ++line;
        if (!input_ln (change_file))
        {
            err.err_print ("! Change file ended after @x");
            return false;
        }
    }
    while (limit <= 0);

    return true;
}

// section 131

void
copy_line_to_change_buffer ()
{
    change_limit = limit;
    std::copy (buffer.begin (), buffer.begin () + limit, change_buffer.begin ());
}

// section 132

bool
verify_possible_y_line (int non_matching_lines);

void
check_change ()
{
    if (lines_dont_match ())
        return;

    int non_matching_lines = 0;

    while (true)
    {
        change_changing ();  // now it's true
        ++line;

        if (!input_ln (change_file))
        {
            err.err_print ("! Change file ended before @y");
            change_limit = 0_r;
            change_changing ();  // false again
            return;
        }

        if (!verify_possible_y_line (non_matching_lines))
            return;

        copy_line_to_change_buffer ();
        change_changing ();  // now it's false
        ++line;

        if (!input_ln (web_file))
        {
            err.err_print ("! WEB file ended during a change");
            input_has_ended = true;
            return;
        }

        if (lines_dont_match ())
        {
            ++non_matching_lines;
        }
    }
}

// section 133

/// If the current line starts with @y, report any discrepancies and return false
/// Returns true if everything is ok, false if there was an issue
bool
verify_possible_y_line (int non_matching_lines)
{
    if (line < 2 || buffer [0_r] != u8'@')
        return true;

    switch (get_change_control_letter ())
    {
    case u8'y':
        if (non_matching_lines > 0)
        {
            loc = 2_r;
            err.err_print ("! Hmm... {} of the preceding lines failed to match", non_matching_lines);
        }
        return false;

    case u8'x':
    case u8'z': loc = 2_r; err.err_print ("! Where is the matching @y?");
    }

    return true;
}

// section 134

void
initialize_input_system ()
{
    open_input ();

    line       = 0;
    other_line = 0;
    changing   = true;

    prime_the_change_buffer ();
    change_changing ();

    limit           = 0_r;
    loc             = 1_r;

    buffer [0_r]    = u8' ';
    input_has_ended = false;
}

// section 135, 136

void
read_from_change_file ();

void
get_line ()
{
    while (true)
    {
        if (changing)
        {
            read_from_change_file ();
        }

        if (!changing)
        {
            ++line;
            if (!input_ln (web_file))
            {
                input_has_ended = true;
            }
            else if (change_limit > 0)
            {
                check_change ();
            }

            if (changing)
                continue;
        }

        break;
    }

    loc            = 0_r;
    buffer [limit] = u8' ';
}

// section 137

void
read_from_change_file ()
{
    ++line;
    if (!input_ln (change_file))
    {
        err.err_print ("\n! Change file ended without @z");

        buffer [0_r] = u8'@';
        buffer [1_r] = u8'z';
        limit        = 2_r;
    }

    if (limit < 2 || buffer [0_r] != u8'@')
        return;

    switch (get_change_control_letter ())
    {
    case u8'z':
        prime_the_change_buffer ();
        change_changing ();
        break;

    case u8'x':
    case u8'y': loc = 2_r; err.err_print ("! Where is the matching @z?");
    }
}

// section 138

void
check_read_all_changes ()
{
    if (change_limit != 0)
    {
        std::copy (change_buffer.begin (), change_buffer.begin () + change_limit, buffer.begin ());

        limit    = change_limit;
        changing = true;
        line     = other_line;
        loc      = change_limit;
        err.err_print ("! Change file entry did not match");
    }
}

// section 139

/// control code of no interest to TANGLE
constexpr auto ignore       = ascii_code_t {0};
constexpr auto control_text = ascii_code_t {0x83};  /// control code for ‘@t’, ‘@^’, etc.
constexpr auto format       = ascii_code_t {0x84};  /// control code for ‘@f’
constexpr auto definition   = ascii_code_t {0x85};  /// control code for ‘@d’
constexpr auto begin_pascal = ascii_code_t {0x86};  /// control code for ‘@p’
constexpr auto module_name  = ascii_code_t {0x87};  /// control code for ‘@<’
constexpr auto new_module   = ascii_code_t {0x88};  /// control code for ‘@ ’ and ‘@*’

// Declared in module 171 what needed here already
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
        if (loc > limit)  // line ended
        {
            get_line ();
            if (input_has_ended)
                return new_module;
        }

        // Put @ as marker so we don't have to check also for the end
        buffer [limit + 1_r] = u8'@';
        while (buffer [loc] != u8'@') { ++loc; }  // find the next marker

        // If we find a @ (other than our own marker) we check the respective control code
        if (loc <= limit)
        {
            loc += 2_r;
            auto ascii = buffer [loc - 1_r];
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
        if (loc > limit)
        {
            get_line ();
            if (input_has_ended)
            {
                err.err_print ("! Input ended in mid-comment");
                return;
            }
        }

        ascii_code_t c = buffer [loc++];

        if (c == u8'@')
        {
            c = buffer [loc];
            if (c == u8' ' || c == tab_mark || c == u8'*')
            {
                err.err_print ("! Section ended in mid-comment");
                --loc;
                return;
            }
            ++loc;
        }
        else if (c == u8'\\' && buffer [loc] != u8'@')
        {
            ++loc;
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
name_t * cur_module;
bool    scanning_hex = false;  /// are we scanning a hexadecimal constant

// section 145 - 155
ascii_code_t
get_identifier (ascii_code_t c);
ascii_code_t
get_preprocessed_string ();
void
scan_module_name ();

void
compress (ascii_code_t &c, ascii_code_t compressed)
{
    if (loc <= limit)
    {
        c = compressed;
        loc++;
    }
}

bool
compress_if (ascii_code_t &c, ascii_code_t second, ascii_code_t pattern, ascii_code_t compressed)
{
    if (second == pattern)
    {
        compress (c, compressed);
        return true;
    }

    return false;
}

uint8_t
get_next ()
{
    while (true)
    {
        if (loc > limit)
        {
            get_line ();
            if (input_has_ended)
                return new_module;
        }
        ascii_code_t c = buffer [loc++];

        if (scanning_hex)
        {
            if (is_hex (c))
                return c;

            scanning_hex = false;
        }

        if (is_alpha (c))
            return get_identifier (c);

        ascii_code_t cc = buffer [loc];
        switch (c)
        {
        case u8'"': return get_preprocessed_string ();

        case u8'@':
            c = control_code (buffer [loc]);
            ++loc;
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

                if (buffer [loc - 1_r] != u8'>')
                {
                    err.err_print ("! Improper @ within control text");
                }

                continue;
            }
            return c;

            // section 147

        case u8'.': compress_if (c, cc, '.', double_dot) || compress_if (c, cc, ')', u8']'); return c;

        case u8':': compress_if (c, cc, '=', left_arrow); return c;

        case u8'=': compress_if (c, cc, '=', equivalence_sign); return c;

        case u8'>': compress_if (c, cc, '=', greater_or_equal); return c;

        case u8'<':
            compress_if (c, cc, '=', less_or_equal) || compress_if (c, cc, '>', not_equal);
            return c;

        case u8'('   : compress_if (c, cc, '*', begin_comment) || compress_if (c, cc, '.', u8'['); return c;

        case u8'*'   : compress_if (c, cc, ')', end_comment); return c;

        case u8' '   :
        case tab_mark: continue;

        case u8'{'   : skip_comment (); continue;

        case u8'}'   : err.err_print ("! Extra }}"); continue;

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
    if (loc > 1
        && (c == u8'E' || c == u8'e')
        && is_digit (buffer [loc - 2_r]))  // the char before the current
    {
        c = 0;
    }

    if (c != 0)
    {
        ascii_code_t d;
        --loc;
        auto id_first = loc;
        do
        {
            ++loc;
            d = buffer [loc];
        }
        while (is_alphanumeric (d) || d == u8'_');

        if (loc > id_first + 1)
        {
            c          = identifier;
            current_id = {&buffer [id_first], static_cast<size_t> (loc - id_first)};
        }
    }
    else
    {
        c = u8'E';
    }
    return c;
}

// section 149
ascii_code_t
get_preprocessed_string ()
{
    ascii_code_t d;
    double_chars  = 0_r;
    auto id_first = loc - 1_r;

    do
    {
        d = buffer [loc++];
        if (d == u8'"' || d == u8'@')
        {
            if (buffer [loc] == d)
            {
                ++loc;
                d = 0;
                ++double_chars;
            }
            else if (d == u8'@')
            {
                err.err_print ("! Double @ sign missing");
            }
        }
        else if (loc > limit)
        {
            err.err_print ("! String constant didn't end");
            d = u8'"';
        }
    }
    while (d != u8'"');

    current_id = {&buffer [id_first], static_cast<size_t> (loc - 1 - id_first)};
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
            cur_module = &name_mgr.lookup_prefix ({&mod_text[1_r], static_cast <size_t> (k) - 3});
        }
        else
        {
            cur_module = &name_mgr.lookup_module ({&mod_text [1_r], k});
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
        if (loc > limit)  // next line
        {
            get_line ();
            if (input_has_ended)
            {
                err.err_print ("! Input has ended in section name");
                break;
            }
        }

        d = buffer [loc];
        if (d == u8'@')
        {
            d = buffer [loc + 1_r];
            if (d == u8'>')
            {
                loc += 2_r;
                break;
            }
            if (d == u8' ' || d == tab_mark || d == u8'*')
            {
                err.err_print ("! Section name didn't end");
                break;
            }
            ++k;
            mod_text [k] = u8'@';
            ++loc;
        }

        ++loc;
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
        err.terminal ().print_nl ("! Section name too long: ");
        err.terminal ().print ({&mod_text.data ()[1], 25});
        err.terminal ().print ("...");
        err.mark_harmless ();
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
        err.err_print ("! Omit semicolon in numeric definition");
        return scan_numeric_cases::consumed;

    default:
        err.err_print ("! Improper numeric definition will be flushed");
        do { next_control = skip_ahead (); }
        while (!end_of_definition (next_control));

        if (next_control == module_name)  // we want to scan the module name too
        {
            loc -= 2_r;
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
        err.err_print ("! Value too big: ", accumulator);
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
                err.err_print ("! Extra )");
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
                text_mgr.append_to_next_new (0xA800 + name_mgr.index_of (*cur_module));
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
                err.err_print (
                    "! @ {} is ignored in Pascal text",
                    convert_to_output (buffer [loc - 1_r]));
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

    return text_mgr.storage.add_next_new ();
}

// section 166

void
ensure_parantheses_balance (int &balance)
{
    if (balance > 0)
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
            if (buffer [loc] == u8'@')
            {
                ++loc;  // store only one @
            }
            else
            {
                err.err_print ("! You should double @ signs in strings");
            }
        }

        if (loc == limit)
        {
            err.err_print ("! String didn't end");
            buffer [loc]       = '\'';
            buffer [loc + 1_r] = 0;
        }

        b = buffer [loc++];
        if (b == u8'\'')
        {
            if (buffer [loc] != u8'\'')
                break;

            ++loc;
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
    buffer [limit + 1_r] = u8'@';

    while (true)
    {
        if (buffer [loc] == u8'@')
        {
            if (loc < limit)
            {
                if (buffer [loc + 1_r] == u8'@')
                {
                    text_mgr.append_to_next_new (U'@');
                    loc += 2_r;
                    continue;
                }
            }
        }
        else
        {
            text_mgr.append_to_next_new (static_cast<text_manager::char_type> (buffer [loc++]));
            continue;
        }

        break;
    }

    if (loc >= limit)
    {
        err.err_print ("! Verbatim string didn't end");
    }
    else if (buffer [loc + 1_r] != u8'>')
    {
        err.err_print ("! You shouldn't double @ signs in verbatim strings");
    }

    loc += 2_r;
    text_mgr.append_to_next_new (verbatim);

}

// section 170

void
define_macro (ilk_value type)
{
    auto &name = name_mgr.lookup (type, current_id);
    auto &replacement_text = scan_replacement (type);
    name.set_replacement_text (&replacement_text);
    replacement_text.set_continuation (&text_mgr.storage.record_0());
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
                loc -= 2_r;
                next_control = get_next ();
            }
        }

        if (next_control != definition)
            return;

        next_control = get_next ();  // get identifier name
        if (next_control != identifier)
        {
            err.err_print ("! Definition flushed must start with identifier of length > 1");
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
                        err.err_print ("! Use == for macros");
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
    name_t * p = nullptr;
    switch (next_control)
    {
    case begin_pascal: break;
    case module_name:
        p = cur_module;

        do { next_control = get_next (); }
        while (next_control == u8'+');

        if (next_control != u8'=' && next_control != equivalence_sign)
        {
            err.err_print ("! Pascal text flushed, = sign is missing");
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

    if (!p)  // unnamed module
    {
        last_unnamed->set_continuation (&replacement_text);
        last_unnamed = &replacement_text;
    }
    else if (p -> replacement_text())
    {
        p -> replacement_text () -> append_continuation (replacement_text);
    }
    else
    {
        p -> set_replacement_text (&replacement_text);
    }

}

// section 179, 180, 181: debugging, left out for now

// section 182

void
initialize ()
{
    // section 10
    // section 14, 17
    // section 18
    // section 21 nothing tbd

    // section 26
    open_output ();

    // section 42
    name_mgr.initialize(config::max_bytes, config::max_names);
    text_mgr.initialize(config::max_toks, config::max_texts);

    string_ptr     = 256_r;
    pool_check_sum = 271828;

    // section 46
    // section 48
    // section 52
    // section 71
    last_unnamed    = &text_mgr.storage.record_0();
    text_mgr.storage.record_0 ().set_continuation (&text_mgr.storage.record_0 ());

    // section 144
    scanning_hex = false;

    // section 152
    mod_text [0_r] = u8' ';

    // section 180 nothing tbd
}

// section 182
int
tangle (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name)
{
    web_file.assign (web_file_name);
    change_file.assign (change_file_name);
    pascal_file.assign (pascal_file_name);
    pool.assign (pool_file_name);
        
    initialize ();
    initialize_input_system ();
    term.print_ln ("{}", config::banner);

    err.set_print_error_location (print_error_location_input);
    module_count = 0;

    do { next_control = skip_ahead (); }
    while (next_control != new_module);

    while (!input_has_ended) { scan_module (); }

    check_read_all_changes ();
    err.set_print_error_location (print_error_location_output);

    output_compressed_tables (term);
    if (string_ptr > 256)
    {
        term.print_nl ("{} strings written to string pool file.", string_ptr - 256);
        pool.write ('*');

        char digit_buffer [config::max_digits];
        std::to_chars (digit_buffer, std::end (digit_buffer), pool_check_sum);
        for (size_t i = 0; i < 9; ++i) { write (pool, digit_buffer [i]); }
        pool.write_line ();
    }

    web_file.close ();
    change_file.close ();
    pascal_file.close ();
    pool.close ();

    return err.exit_code ();
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
