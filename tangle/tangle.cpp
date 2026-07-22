#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <print>
#include <vector>

#include "utility/between.h"

#include "pascal/array.h"
#include "pascal/range.h"
#include "pascal/text_file.h"

#include "character.h"
#include "error.h"
#include "out_buffer.h"
#include "out_processor.h"
#include "tangle.h"
#include "terminal.h"

static_assert (CHAR_BIT == 8, "Error: This codebase strictly requires an 8-bit char architecture");

using namespace std::literals;
using pascal::operator""_r;

// section 1
constexpr auto banner = "This is a C++ reimplementation of TANGLE, Version 4.6"sv;

// section 2 - 7 n/a
// initialize will be defined in section 182 just before main because no variables have been declared,
// yet

// section 8
constexpr size_t buf_size = 100;  /// maximum length of input line
constexpr size_t max_bytes
    = 45000;  /// number of bytes in identifiers, strings, and module names; must be < 65536
constexpr size_t max_toks     = 65000;  /// number of bytes in compressed Pascal code; must be < 65536
constexpr size_t max_names    = 4000;   /// number of identifiers, strings, module names; must be < 10240
constexpr size_t max_texts    = 2000;   /// number of replacement texts, must be < 10240
constexpr size_t hash_size    = 353;    /// should be prime
constexpr size_t longest_name = 400;    /// module names shouldn’t be longer than this
constexpr size_t line_length  = 72;     /// lines of Pascal output have at most this many characters
constexpr size_t stack_size   = 50;     /// number of simultaneous levels of macro expansion
constexpr size_t max_id_length
    = 12;  /// long identifiers are chopped to this length, which must not exceed line length
constexpr size_t unambig_length = 7;  /// identifiers must be unique if chopped to this length

// additional declarations to avoid magic constants
constexpr size_t max_modules = 027777;  /// 0x3FFF

// the maximum number of digits an int can have (in tangle)
constexpr size_t max_digits  = 11;

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

void
print (terminal &term, ascii_code_t c)
{ term.print (convert_to_output(c)); }


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
using buf_index_t = pascal::int_range<0, buf_size>;
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
        if (limit == buf_size)
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
        print (term, ch == tab_mark ? u8' ' : ch);
    }
    term.print_nl ("{:>{}}", "", int {l});

    // print not yet read characters
    for (auto k = l; k < limit; ++k)
    {
        auto ch = buffer [k];
        print (term, ch);
    }
    term.print (' ');
}

// section 33

// defined in later section but needed here already
auto out_buf = out_buffer {line_length, pascal_file};

void
print_error_location_output (terminal &term)
{
    term.print_ln (". (l.{})", out_buf.current_line());
    for (auto c : out_buf.temporary_view()) { print (term, c); }
    term.print ("... ");
}

// section 34
// section 35
// section 36
// section 37
// We use uint8_t and uint16_t instead of eight_bits and sixteen_bits
// section 38

/// we multiply the byte capacity by approximately this amount
auto constexpr ww     = 2_r;
auto constexpr zz     = 3_r;  /// we multiply the token capacity by approximately this amount

using name_pointer_t  = pascal::int_range<0, max_names>;  /// identifies a name
using text_pointer_t  = pascal::int_range<0, max_texts>;
using byte_pointer_t  = pascal::int_range<0, max_bytes>;
using token_pointer_t = pascal::int_range<0, max_toks>;
using token_bank_t    = pascal::int_range<0, zz - 1_r>;
using byte_bank_t     = pascal::int_range<0, ww - 1_r>;
using byte_mem_bank_t = pascal::array<byte_pointer_t, ascii_code_t>;

auto byte_mem         = pascal::array<byte_bank_t, byte_mem_bank_t> {};  /// characters of names
auto tok_mem = pascal::array<token_bank_t, pascal::array<token_pointer_t, ascii_code_t>> {};  /// tokens
auto byte_start = pascal::array<name_pointer_t, uint16_t> {};  /// directory into byte mem
auto tok_start  = pascal::array<text_pointer_t, uint16_t> {};  /// directory into tok mem
auto link       = pascal::array<name_pointer_t, uint16_t> {};  /// hash table or tree links
auto ilk        = pascal::array<name_pointer_t, uint16_t> {};  /// type codes or tree links
auto equiv      = pascal::array<name_pointer_t, uint16_t> {};  /// info corresponding to names
auto text_link  = pascal::array<text_pointer_t, uint16_t> {};  /// relates replacement texts

auto
locate_byte_mem (name_pointer_t p)
{
    auto w = p % ww;
    return std::tuple<decltype (w), byte_mem_bank_t &, byte_pointer_t> {w, byte_mem [w], byte_start [p]};
}

// section 39

/// length of a name
auto
length (name_pointer_t index)
{ return byte_start [index + ww] - byte_start [index]; }

// name_pointer is name_index_t and we needed to define it alreay in section 38

// section 40

/// first unused position in byte_start
auto name_ptr   = name_pointer_t {1};
auto string_ptr = name_pointer_t {256};  /// next number to be given to a string of length > 1
auto byte_ptr   = pascal::array<byte_bank_t, byte_pointer_t> {};  /// first unused position in byte mem
auto pool_check_sum = 271828;  /// sort of a hash for the whole string pool

// section 41, 42, 43 not required (global arrays are zero-initialized in C++),
// other initializers already given in section 40

// section 44

/// identifies a replacement text
auto text_ptr = text_pointer_t {1};
auto tok_ptr  = pascal::array<token_bank_t, token_pointer_t> {};  /// first unused position in tok_start
auto z        = token_bank_t {1};                                 /// current segment of tok_mem

// section 45, 46 not required

// section 47

enum ilk_value
{
    normal,     /// ordinary identifiers
    numeric,    /// numeric macros and strings
    simple,     /// simple macros
    parametric  /// parametric macros
};

// section 48
/// left link in binary search tree for module names
auto &llink = link;
auto &rlink = ilk;  /// right link in binary search tree for module names

// section 49

void
print_id (terminal term, name_pointer_t p)
{
    if (p >= name_ptr) [[unlikely]]
    {
        term.print ("IMPOSSIBLE");
    }
    else
    {
        const auto end = byte_start [p + ww];
        for (auto [_, bank, k] = locate_byte_mem (p); k < end; ++k)
        {
            print (term, bank [k]);
        }
    }
}

// section 50
using hash_index_t    = pascal::int_range<0, hash_size>;
using chopped_index_t = pascal::int_range<0, unambig_length>;

auto id_first         = buf_index_t {};
auto id_loc           = buf_index_t {};
auto double_chars     = buf_index_t {};
auto hash             = pascal::array<hash_index_t, uint16_t> {};
auto chop_hash        = pascal::array<hash_index_t, uint16_t> {};
auto chopped_id       = pascal::array<chopped_index_t, ascii_code_t> {};

/// convenience function because this value is used very often
auto
id_length ()
{ return buf_index_t {id_loc - id_first}; }

// section 51, 52 not required

// section 53

auto
compute_hash_code () -> hash_index_t;
auto
compute_name_location (hash_index_t h) -> name_pointer_t;
void
update_tables (name_pointer_t p, ilk_value t);

/// Finds current identifier if it exists or stores it.
auto
id_lookup (ilk_value t) -> name_pointer_t
{
    hash_index_t   h = compute_hash_code ();
    name_pointer_t p = compute_name_location (h);

    if (p == name_ptr || t != normal)
    {
        update_tables (p, t);
    }

    return p;
}

// section 54

/// computes the hash of the current id
auto
compute_hash_code () -> hash_index_t
{
    auto h = hash_index_t {static_cast<uint8_t> (buffer [id_first])};
    for (auto i = buf_index_t {id_first + 1}; i < id_loc; ++i)
    {
        h = hash_index_t {(h + h + buffer [i]) % hash_size};
    };
    return h;
}

// section 55

auto
compare_with_current_identifier (name_pointer_t p) -> bool;

/// Finds the index of the current id (in buffer) into byte_start.
///
/// The hash list is implemented by storing the start of each bucket in hash
/// and the linked list for each bucket implicitly in link[p] (there must be
/// at most one successor for each p)
auto
compute_name_location (hash_index_t h) -> name_pointer_t
{
    auto p   = name_pointer_t {hash [h]};
    auto len = id_length ();
    while (p != 0)
    {
        if (length (p) == len && compare_with_current_identifier (p))
            return p;

        p = name_pointer_t {link [p]};
    }
    p = name_ptr;

    // insert p at beginning of hash list
    link [p] = hash [h];
    hash [h] = p;

    return p;
}

// section 56

auto
compare_with_current_identifier (name_pointer_t p) -> bool
{
    auto [_, bank, k] = locate_byte_mem (p);
    return std::memcmp (&buffer [id_first], &bank [k], id_length ()) == 0;
}

// section 57

auto
compute_secondary_hash () -> hash_index_t;
void
double_definition_error (name_pointer_t p, ilk_value t, hash_index_t h);
void
add_new_name (name_pointer_t p, ilk_value t, hash_index_t h);

/// Update the tables and check for possible errors
void
update_tables (name_pointer_t p, ilk_value t)
{
    hash_index_t h = 0_r;

    if ((p != name_ptr && t != normal && ilk [p] == normal)
        || (p == name_ptr && t == normal && buffer [id_first] != u8'"'))
    {
        h = compute_secondary_hash ();
    }

    if (p != name_ptr)
    {
        double_definition_error (p, t, h);
    }
    else
    {
        add_new_name (p, t, h);
    }
}

// section 58

/// Computes secondary hash and sets chopped_id to the chopped id
auto
compute_secondary_hash () -> hash_index_t
{
    chopped_index_t s = 0_r;
    hash_index_t    h = 0_r;
    for (auto i = id_first; i < id_loc; ++i)
    {
        if (s == unambig_length)
            break;

        auto ch = buffer [i];

        if (ch == u8'_')
            continue;

        if (ch >= u8'a')
        {
            ch -= 040;
        }

        chopped_id [s] = ch;
        h              = hash_index_t {(h + h + ch) % hash_size};
        ++s;
    }
    chopped_id [s] = 0;

    return h;
}

// section 59

void
remove_from_secondary_hash_table (name_pointer_t p, hash_index_t h);

void
double_definition_error (name_pointer_t p, ilk_value t, hash_index_t h)
{
    if (ilk [p] == normal)  // We have seen p before it was used
    {
        if (t == numeric)  // We don't allow numeric macros to be defined after their first use
        {
            err.err_print ("! This identifier has already appeared");

            // nevertheless we treat it as numeric from now on
            // numeric macros are not stored in secondary hash table
            remove_from_secondary_hash_table (p, h);
        }

        // We only make it a message for numeric because it might break the internal math
        // All other cases are not problematic
    }
    else
    {
        err.err_print ("! This identifier was defined before");
    }

    // the second definition wins: we force a new ilk on p
    ilk [p] = t;
}

// section 60

void
remove_from_secondary_hash_table (name_pointer_t p, hash_index_t h)
{
    auto q = name_pointer_t {chop_hash [h]};
    if (q == p)
    {
        chop_hash [h] = equiv [p];
    }
    else
    {
        while (equiv [q] != p) { q = name_pointer_t {equiv [q]}; }
        equiv [q] = equiv [p];
    }
}

// section 61

void
update_secondary_hash (name_pointer_t p, hash_index_t h);
void
add_new_string (name_pointer_t p);

void
add_new_name (name_pointer_t p, ilk_value t, hash_index_t h)
{
    auto first_char = buffer [id_first];

    if (t == normal && first_char != u8'"')
    {
        update_secondary_hash (p, h);
    }

    auto [w, bank, _] = locate_byte_mem (name_ptr);
    auto k            = byte_ptr [w];
    auto length       = id_length ();
    auto k_final      = k + length;

    if (k_final > max_bytes)
        err.overflow ("byte memory");

    if (name_ptr > max_names - ww)
        err.overflow ("name");

    std::memcpy (&bank [k], &buffer [id_first], length);

    byte_ptr [w]               = k_final;
    byte_start [name_ptr + ww] = k_final;
    ++name_ptr;

    if (first_char != u8'"')
    {
        ilk [p] = t;
    }
    else
    {
        add_new_string (p);
    }
}

// section 62
void
check_conflicting_names (name_pointer_t q);

void
update_secondary_hash (name_pointer_t p, hash_index_t h)
{
    auto q = name_pointer_t {chop_hash [h]};
    while (q != 0)
    {
        check_conflicting_names (q);
        q = name_pointer_t {equiv [q]};
    }

    // put p at front of secondary hash list
    equiv [p]     = chop_hash [h];
    chop_hash [h] = p;
}

// section 63

/// Checks whether name indicated by q has same chopped id as the the value in chopped_id
void
check_conflicting_names (name_pointer_t q)
{
    chopped_index_t s     = 0_r;

    auto            end   = byte_pointer_t {byte_start [q + ww]};
    auto [_, bank, start] = locate_byte_mem (q);

    auto k                = start;
    for (; k < end; ++k)
    {
        if (s == unambig_length)
            break;

        auto ch = bank [k];
        if (ch == u8'_')
            continue;

        if (ch >= u8'a')
        {
            ch -= 040;
        }

        if (chopped_id [s] != ch)
            return;

        ++s;
    }

    if (k == end && chopped_id [s] != 0)
        return;

    err.terminal ().print_nl ("! Identifier conflict with ");
    for (k = start; k < end; ++k) { print (err.terminal (), bank [k]); }
    err.error ();
}

// section 64

constexpr auto checksum_prime = (1 << 29) - 73;

void
add_to_checksum (int value)
{
    pool_check_sum += pool_check_sum + value;
    while (pool_check_sum > checksum_prime) { pool_check_sum -= checksum_prime; }
}

void
add_new_string (name_pointer_t p)
{
    ilk [p]     = numeric;
    auto length = id_length ();

    if (length - double_chars == 2)  // single-character string
    {
        equiv [p] = buffer [id_first + 1_r] + 0100000;
    }
    else
    {
        equiv [p] = string_ptr + 0100000;
        length -= (double_chars + 1_r);
        if (length > 99)
        {
            err.err_print ("! Preprocessed string is too long");
        }
        ++string_ptr;

        // output length
        write (pool, u8'0' + length / 10);
        write (pool, u8'0' + length % 10);

        add_to_checksum (length);

        auto i = buf_index_t {id_first + 1};
        while (i < id_loc)
        {
            auto ch = buffer [i];
            write (pool, ch);
            add_to_checksum (ch);
            if (ch == u8'"' || ch == u8'@')
            {
                i += 2_r;  // omit second appearance of doubled character
            }
            else
            {
                ++i;
            }
        }
        pool.write_line ();
    }
}

// section 65

/// Index in one name
using inname_index_t = pascal::int_range<0, longest_name>;
auto mod_text        = pascal::array<inname_index_t, ascii_code_t> {};  /// name being sought for

// section 66

enum comparison_result
{
    less,
    equal,
    greater,
    prefix,
    extension
};

auto
compare_module_names (uint16_t length, name_pointer_t p) -> comparison_result;
auto
add_module_name (uint16_t length, comparison_result &c, name_pointer_t q) -> name_pointer_t;

auto
module_lookup (uint16_t length) -> name_pointer_t
{
    auto c = greater;
    auto q = name_pointer_t {0};
    auto p = name_pointer_t {rlink [0_r]};

    while (p != 0)
    {
        c = compare_module_names (length, p);
        q = p;
        if (c == less)
        {
            p = name_pointer_t {llink [q]};
        }
        else if (c == greater)
        {
            p = name_pointer_t {rlink [q]};
        }
        else
        {
            if (c != equal)
            {
                err.err_print ("! Incompatible section names");
                return 0_r;
            }
            return p;
        }
    }

    return add_module_name (length, c, q);
}

// section 67

auto
add_module_name (uint16_t length, comparison_result &c, name_pointer_t q) -> name_pointer_t
{
    auto [w, bank, _] = locate_byte_mem (name_ptr);
    auto k            = byte_ptr [w];
    auto k_final      = byte_pointer_t {k + length};

    if (k_final > max_bytes)
        err.overflow ("byte memory");

    if (name_ptr > max_names - ww)
        err.overflow ("name");

    auto p = name_ptr;
    if (c == less)
    {
        llink [q] = p;
    }
    else
    {
        rlink [q] = p;
    }

    llink [p] = 0;
    rlink [p] = 0;

    c         = equal;
    equiv [p] = 0;

    std::memcpy (&bank [k], &mod_text [1_r], length);
    byte_ptr [w]               = k_final;
    byte_start [name_ptr + ww] = k_final;
    ++name_ptr;

    return p;
}

// section 68

auto
compare_module_names (uint16_t length, name_pointer_t p) -> comparison_result
{
    comparison_result c   = equal;
    inname_index_t    j   = 1_r;

    uint16_t          end = byte_start [p + ww];
    auto [_, bank, k]     = locate_byte_mem (p);

    while (k < end && j <= length && mod_text [j] == bank [k])
    {
        ++k;
        ++j;
    }

    if (k == end)
        return j > length ? equal : extension;

    if (j > length)
        return prefix;

    if (mod_text [j] < bank [k])
        return less;

    return greater;
}

// section 69

auto
prefix_lookup (uint16_t length) -> name_pointer_t
{
    auto resume_node  = name_pointer_t {0};
    auto current_node = name_pointer_t {rlink [0_r]};
    auto count        = name_pointer_t {0};
    auto result       = name_pointer_t {0};

    while (current_node != 0)
    {
        auto c = compare_module_names (length, current_node);
        if (c == less)
        {
            current_node = name_pointer_t {llink [current_node]};
        }
        else if (c == greater)
        {
            current_node = name_pointer_t {rlink [current_node]};
        }
        else
        {
            result = current_node;
            ++count;
            resume_node  = name_pointer_t {rlink [current_node]};
            current_node = name_pointer_t {llink [current_node]};
        }

        if (current_node == 0)
        {
            current_node = resume_node;
            resume_node  = 0_r;
        }
    }

    if (count != 1_r)
    {
        if (count == 0_r)
        {
            err.err_print ("! Name does not match");
        }
        else
        {
            err.err_print ("! Ambiguous prefix");
        }
    }

    return result;
}

// section 70

/// final text_link in module replacement texts
auto module_flag  = static_cast<uint16_t> (max_texts);
auto last_unnamed = text_pointer_t {0};  /// most recent replacement text of unnamed module

// section 71 not required

// section 72

/// insertion of parameter
constexpr auto param         = ascii_code_t {0};
constexpr auto verbatim      = ascii_code_t {2};     /// @= begins a verbatim Pascal string, @> ends it
constexpr auto force_line    = ascii_code_t {3};     /// @\ forces a new line in the Pascal output
constexpr auto begin_comment = ascii_code_t {011};   /// @{ turns into { or [. in output
constexpr auto end_comment   = ascii_code_t {012};   /// @} turns into } or .] in output
constexpr auto octal         = ascii_code_t {014};   /// @' precedes an octal constant
constexpr auto hex           = ascii_code_t {015};   /// @" preceds a hex constant
constexpr auto double_dot    = ascii_code_t {040};   /// denotes .. in Pascal
constexpr auto check_sum     = ascii_code_t {0175};  /// @$ denotes the string pool check sum
constexpr auto join          = ascii_code_t {0177};  /// @& is the item concatenation operator

// section 73

/// stores high byte, then low byte in tok_mem
void
store_two_bytes (uint16_t x)
{
    if (tok_ptr [z] + 2_r > max_toks)
        err.overflow ("token");

    tok_mem [z][tok_ptr [z]]       = x >> 8;
    tok_mem [z][tok_ptr [z] + 1_r] = x & 0xFF;

    tok_ptr [z] += 2_r;
}

// section 74, 75, 76 implement print_repl for debug mode if that gets included
// section 77 nothing tbd
// section 78

using mod_pointer_t = pascal::int_range<0, max_modules>;

struct output_state
{
    uint16_t       end_field;   /// ending location of replacement text
    uint16_t       byte_field;  /// present location within replacement text
    name_pointer_t name_field;  /// byte_start index for text being output
    text_pointer_t repl_field;  /// tok_start index for text being output
    mod_pointer_t  mod_field;   /// module number or zero if not a module
};

// section 79

/// current output state
auto  cur_state = output_state {};

auto &cur_end   = cur_state.end_field;   /// current ending location in tok mem
auto &cur_byte  = cur_state.byte_field;  /// location of next output byte in tok mem
auto &cur_name  = cur_state.name_field;  /// pointer to current name being expanded
auto &cur_repl  = cur_state.repl_field;  /// pointer to current replacement text
auto &cur_mod   = cur_state.mod_field;   /// current module number being expanded

auto  stack     = pascal::int_range_array<1, stack_size, output_state> {};
auto  stack_ptr = pascal::int_range<0, stack_size> {};

/// section 80

auto zo = token_bank_t {};

// section 81 nothing tbd
// section 82

auto brace_level = uint8_t {};  /// current depth of @{...@} nesting

// section 83

void
initialize_output_stacks ()
{
    stack_ptr   = 1_r;
    brace_level = 0_r;
    cur_name    = 0_r;
    cur_repl    = text_pointer_t {text_link [0_r]};

    // Fix: Intercept Knuth's deliberate out-of-bounds sentinel check safely
    if (cur_repl == module_flag)
    {
        zo       = 0_r;
        cur_byte = 0;
        cur_end  = 0;
    }
    else
    {
        zo       = cur_repl % zz;
        cur_byte = tok_start [cur_repl];
        cur_end  = tok_start [cur_repl + zz];
    }

    cur_mod = 0_r;
}

// section 84

/// suspends the current level
void
push_level (name_pointer_t p)
{
    if (stack_ptr == stack_size)
        err.overflow ("stack");

    stack [stack_ptr++] = cur_state;
    cur_name            = p;
    cur_repl            = text_pointer_t {equiv [p]};
    zo                  = cur_repl % zz;
    cur_byte            = tok_start [cur_repl];
    cur_end             = tok_start [cur_repl + zz];

    if (cur_repl == module_flag)
    {
        zo       = 0_r;
        cur_byte = 0;
        cur_end  = 0;
    }
    else
    {
        zo       = cur_repl % zz;
        cur_byte = tok_start [cur_repl];
        cur_end  = tok_start [cur_repl + zz];
    }

    cur_mod = 0_r;
}

// section 85

void
pop_parameter_stack ();

/// do this when cur_byte reaches cur_end
void
pop_level ()
{
    auto repl = text_pointer_t {text_link [cur_repl]};
    if (repl == 0)  // end of macro expansion
    {
        if (ilk [cur_name] == parametric)
        {
            pop_parameter_stack ();
        }
    }
    else if (repl < module_flag)  // link to a continuation
    {
        cur_repl = repl;  // stay on same level
        zo       = cur_repl % zz;
        cur_byte = tok_start [cur_repl];
        cur_end  = tok_start [cur_repl + zz];
        return;
    }

    if (--stack_ptr > 0)  // go down to previous level
    {
        cur_state = stack [stack_ptr];
        zo        = cur_repl % zz;
    }
}

// section 86

constexpr auto number        = 0200;  /// code returned by get output when next output is numeric
constexpr auto module_number = 0201;  ///  code returned by get output for module numbers
constexpr auto identifier    = 0202;  /// code returned by get output for identifiers

int            cur_val;  /// additional information corresponding to output token

// section 87, 88, 89, 90, 92

void
copy_parameter_to_tok_mem ();

/// returns next token after macro expansion
uint16_t
get_output ()
{
    while (true)  // because we need to restart once in a while
    {
        if (stack_ptr == 0)
            return 0;

        if (cur_byte == cur_end)
        {
            cur_val = -cur_mod;
            pop_level ();
            if (cur_val == 0)
                continue;

            return module_number;
        }

        uint16_t a = tok_mem [zo][token_pointer_t {cur_byte++}];

        if (a < 0200)  // one-byte token
        {
            if (a != param)
                return a;

            // section 92
            // start scanning current macro parameter
            push_level (name_ptr - 1_r);
            continue;
        }

        a = (a - 0200) << 8 | tok_mem [zo][token_pointer_t {cur_byte++}];

        if (a < 024000)  // (0250 - 0200) * 0400
        {
            auto an = name_pointer_t {a};

            // section 89

            switch (ilk [an])
            {
            case normal : cur_val = an; return identifier;

            case numeric: cur_val = equiv [an] - 0100000; return number;

            case simple : push_level (an); continue;

            case parametric:
            {
                // section 90

                while (cur_byte == cur_end && stack_ptr > 0) { pop_level (); }

                if (stack_ptr == 0 || tok_mem [zo][token_pointer_t {cur_byte}] != u8'(')
                {
                    err.terminal ().print_nl ("! No parameter given for ");
                    print_id (err.terminal (), an);
                    err.error ();
                    continue;
                }

                copy_parameter_to_tok_mem ();

                equiv [name_ptr] = text_ptr;
                ilk [name_ptr]   = simple;
                auto w           = name_ptr % ww;
                auto k           = byte_ptr [w];

                if (name_ptr > max_names - ww)
                    err.overflow ("name");

                byte_start [name_ptr + ww] = k;
                ++name_ptr;

                if (text_ptr > max_texts - zz)
                    err.overflow ("text");

                text_link [text_ptr]      = 0;
                tok_start [text_ptr + zz] = tok_ptr [z];
                ++text_ptr;
                z = text_ptr % zz;

                push_level (an);
                continue;
            }

            default: err.confusion ("output");
            }
        }

        if (a < 050000)
        {
            // section 88

            a -= 024000;
            auto an = name_pointer_t {a};
            if (equiv [an] != 0)
            {
                push_level (an);
            }
            else if (an != 0)
            {
                err.terminal ().print_nl ("! Not present: <");
                print_id (err.terminal (), an);
                err.terminal ().print ('>');
                err.error ();
            }
            continue;
        }

        cur_val = a - 050000;
        a       = module_number;
        cur_mod = mod_pointer_t {cur_val};
        return a;
    }
}

// section 91

void
pop_parameter_stack ()
{
    --name_ptr;
    --text_ptr;
    z           = text_ptr % zz;
    tok_ptr [z] = token_pointer_t {tok_start [text_ptr]};
}

// section 93

/// append replacement text
void
app_repl (uint8_t b)
{
    auto first_free = tok_ptr [z];
    if (first_free == max_toks)
        err.overflow ("token");

    tok_mem [z][first_free] = b;
    tok_ptr [z]             = first_free + 1_r;
}

/// .
void
copy_parameter_to_tok_mem ()
{
    uint16_t balance = 1;  /// excess of ( versus ) while copying a parameter
    ++cur_byte;
    while (true)
    {
        uint8_t b = tok_mem [zo][token_pointer_t {cur_byte++}];
        if (b == param)
        {
            store_two_bytes (name_ptr + 077777);
        }
        else
        {
            if (b >= 0200)
            {
                app_repl (b);
                b = tok_mem [zo][token_pointer_t {cur_byte++}];
            }
            else
            {
                switch (b)
                {
                case u8'(': ++balance; break;

                case u8')':
                    if (--balance == 0)
                        return;
                    break;

                case u8'\'':
                    do
                    {
                        app_repl (b);
                        b = tok_mem [zo][token_pointer_t {cur_byte++}];
                    }
                    while (b != u8'\'');  // copy string, don't change balance
                    break;
                }
            }
            app_repl (b);
        }
    }
}

// section 94
// section 95

auto out_proc = out_processor {out_buf};

// section 96
// section 97

void
on_new_line (int line)
{
    if (line % 100 == 0)
    {
        term.print ('.');
        if (line % 500 == 0)
        {
            term.print ("{}", line);
        }
        term.update ();
    }
}

void
on_line_truncated ()
{ err.err_print ("! Long line must be truncated"); }

void
on_missing_sign_between_numbers ()
{ err.err_print ("! Two numbers occurred without a sign between them"); }

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
send_the_output ();

void
output_compressed_tables (terminal &term)
{
    if (text_link [0_r] == 0_r)
    {
        err.terminal ().print_nl ("! No output was specified.");
        err.mark_harmless ();
    }
    else
    {
        term.print_nl ("Writing the output file");
        term.update ();
        initialize_output_stacks ();
        send_the_output ();
        out_buf.flush_last_line ();
        if (brace_level != 0)
        {
            err.err_print ("! Program ended at brace level {}", brace_level);
        }
        term.print_nl ("Done.");
    }
}

// section 113

enum class send_output_cases
{
    not_consumed,
    consumed,
    reswitch
};

auto
send_output_identifier (ascii_code_t cur_char) -> bool;
auto
send_output_constant (ascii_code_t &cur_char) -> send_output_cases;
auto
send_output_operator (ascii_code_t cur_char) -> bool;
auto
send_output_brace_case (ascii_code_t cur_char) -> bool;
void
send_output_string ();
void
send_output_verbatim_string ();

void
send_output_one_char (ascii_code_t cur_char)
{
    while (true)
    {
        if (send_output_identifier (cur_char))
            return;

        switch (send_output_constant (cur_char))
        {
        case send_output_cases::not_consumed: break;
        case send_output_cases::consumed    : return;
        case send_output_cases::reswitch    : continue;
        }

        if (send_output_operator (cur_char))
            return;

        // section 115
        // other printable characters
        switch (cur_char)
        {
        case u8'!':
        case u8'"':
        case u8'#':
        case u8'$':
        case u8'%':
        case u8'&':
        case u8'(':
        case u8')':
        case u8'*':
        case u8',':
        case u8'/':
        case u8':':
        case u8';':
        case u8'<':
        case u8'=':
        case u8'>':
        case u8'?':
        case u8'@':
        case u8'[':
        case u8'\\':
        case u8']':
        case u8'^':
        case u8'_':
        case u8'`':
        case u8'{':
        case u8'|' : out_proc.process_single_char (cur_char); return;
        }

        if (send_output_brace_case (cur_char))
            return;

        switch (cur_char)
        {
        case 0    : return;

        case u8'+': 
            out_proc.process_sign (+1); return;
        case u8'-': 
            out_proc.process_sign (-1); return;

        case u8'\'':
            send_output_string ();
            cur_char = static_cast<ascii_code_t> (get_output () & 0xFF);
            if (cur_char == '\'')
            {
                out_proc.ensure_no_line_break();
            }
            continue;

        case join:
            out_proc.process_fraction({});
            out_proc.ensure_no_line_break ();
            return;

        case verbatim  : send_output_verbatim_string (); return;

        case force_line: out_proc.force_line_break (); return;

        default        : err.err_print ("! Can't output ASCII code {}", static_cast<uint8_t> (cur_char));
        }
    }
}

void
send_the_output ()
{
    while (stack_ptr > 0) { send_output_one_char (static_cast<ascii_code_t> (get_output () & 0xFF)); }
}

// section 114

auto
send_output_operator (ascii_code_t cur_char) -> bool
{
    switch (cur_char)
    {
    case and_sign        : out_proc.process_identifier (u8"AND"); return true;
    case not_sign        : out_proc.process_identifier (u8"NOT"); return true;
    case set_element_sign: out_proc.process_identifier (u8"IN"); return true;
    case or_sign         : out_proc.process_identifier (u8"OR"); return true;
    case left_arrow      : out_proc.process_string (u8":="); return true;
    case not_equal       : out_proc.process_string (u8"<>"); return true;
    case greater_or_equal: out_proc.process_string (u8">="); return true;
    case equivalence_sign: out_proc.process_string (u8"=="); return true;
    case less_or_equal   : out_proc.process_string (u8"<="); return true;
    case double_dot      : out_proc.process_string (u8".."); return true;
    }

    return false;
}

// section 115 incorporated in section 113

// section 116

auto
send_output_identifier (ascii_code_t cur_char) -> bool
{
    if (is_upper (cur_char))
    {
        out_proc.process_identifier ({&cur_char, 1});
        return true;
    }

    if (is_lower (cur_char))
    {
        cur_char -= 040;
        out_proc.process_identifier ({&cur_char, 1});
        return true;
    }

    if (cur_char == identifier)
    {
        auto   buffer     = std::array<ascii_code_t, max_id_length> {};
        size_t k          = 0_r;
        auto [_, bank, j] = locate_byte_mem (name_pointer_t {cur_val});
        auto end          = byte_start [name_pointer_t {cur_val + ww}];
        while (k < max_id_length && j < end)
        {
            auto ch = bank [j++];
            if (ch >= u8'a')
            {
                ch -= 040;
            }

            if (ch != '_')
            {
                buffer [k++] = ch;
            }
        }

        out_proc.process_identifier({buffer.data (), k});
        return true;
    }

    return false;
}

// Section 117

void
send_output_string ()
{
    auto   buffer     = std::array<ascii_code_t, line_length> {};
    size_t     k      = 0;
    buffer [0]        = u8'\'';
    ascii_code_t ch;
    do
    {
        if (k < line_length - 1)
        {
            ++k;
        }
        buffer [k] = ch = get_output () & 0x7F;
    }
    while (ch != u8'\'' && stack_ptr != 0);

    if (k == line_length - 1)
    {
        err.err_print ("! String too long");
    }
    out_proc.process_string ({buffer.data (), k + 1});
}

// section 118

void
send_output_verbatim_string ()
{
    auto buffer = std::array<ascii_code_t, line_length> {};
    size_t k    = 0;
    ascii_code_t ch;
    do
    {
        buffer [k] = ch = get_output () & 0x7F;
        if (k < line_length - 1)
        {
            ++k;
        }
    }
    while (ch != verbatim && stack_ptr != 0);

    if (k == line_length - 1)
    {
        err.err_print ("! Verbatim string too long");
    }
    out_proc.process_string ({buffer.data (), (size_t) k - 1});
}

// section 119

void
send_out_number (ascii_code_t &cur_char, int base, int limit, bool (*is_valid) (ascii_code_t))
{
    int n = 0;
    do
    {
        int digit_value = (cur_char >= u8'A') ? (cur_char - u8'A' + 10) : (cur_char - u8'0');
        if (n >= limit)
        {
            err.err_print ("! Constant too big");
        }
        else
        {
            n = base * n + digit_value;
        }
        cur_char = get_output () & 0xFF;
    }
    while (is_valid (cur_char));
    out_proc.process_value (n);
}

void
finish_real_constant (ascii_code_t &, bool);

auto
send_output_constant (ascii_code_t &cur_char) -> send_output_cases
{
    if (is_digit (cur_char))
    {
        send_out_number (cur_char, 10, 0xCCCCCCC, is_digit);

        if (cur_char == u8'e')
        {
            cur_char = u8'E';
        }
        if (cur_char == u8'E')
        {
            finish_real_constant (cur_char, false);
        }

        return send_output_cases::reswitch;
    }

    switch (cur_char)
    {
    case check_sum: out_proc.process_value (pool_check_sum); return send_output_cases::consumed;

    case octal:
        cur_char = u8'0';
        send_out_number (cur_char, 8, 0x10000000, is_octal);
        return send_output_cases::reswitch;

    case hex:
        cur_char = u8'0';
        send_out_number (cur_char, 16, 0x8000000, is_hex);
        return send_output_cases::reswitch;

    case number: out_proc.process_value (cur_val); return send_output_cases::consumed;

    case u8'.':
        cur_char          = get_output () & 0xFF;
        if (cur_char == u8'.')
        {
            out_proc.process_string (u8"..");
            return send_output_cases::consumed;
        }
        
        if (is_digit (cur_char))
        {
            finish_real_constant (cur_char, true);
            return send_output_cases::reswitch;
        }
        
        out_proc.process_single_char (u8'.');
        return send_output_cases::reswitch;
    }
    return send_output_cases::not_consumed;
}

// section 120

void
finish_real_constant (ascii_code_t &cur_char, bool start_with_dot)
{  
    auto   buffer = std::array<ascii_code_t, line_length> {};
    size_t k      = 0;
    if (start_with_dot)
    {
        k = 1;
        buffer [0] = u8'.';
    }

    do
    {
        if (k < line_length)
        {
            ++k;
        }

        auto last_char  = cur_char;
        buffer [k - 1]  = cur_char;
        cur_char        = get_output () & 0xFF;

        if (last_char == u8'E' && (cur_char == u8'+' || cur_char == u8'-'))
        {
            if (k < line_length)
            {
                ++k;
            }
            buffer [k - 1] = cur_char;
            cur_char        = get_output () & 0xFF;
        }
        else if (cur_char == u8'e')
        {
            cur_char = u8'E';
        }
    }
    while (cur_char == u8'E' || is_digit (cur_char));

    if (k == line_length)
    {
        err.err_print ("! Fraction too long");
    }

    out_proc.process_fraction({buffer.data (), k});
}

// section 121
auto
send_output_brace_case (ascii_code_t cur_char) -> bool
{
    switch (cur_char)
    {
    case begin_comment: out_proc.process_single_char (brace_level++ == 0 ? u8'{' : u8'['); return true;

    case end_comment:
        if (brace_level > 0)
        {
            out_proc.process_single_char (--brace_level == 0 ? u8'}' : u8']');
        }
        else
        {
            err.err_print ("! Extra @}}");
        }
        return true;

    case module_number:
    {
        constexpr size_t buf_size = max_digits + 3; // digits + 2 braces + 1 colon
        auto             buffer     = std::array<char8_t, buf_size> {};
        auto            *write_ptr  = buffer.data ();
        *write_ptr++                = (brace_level == 0 ? u8'{' : u8'[');
        if (cur_val < 0)
        {
            *write_ptr++ = u8':';
            cur_val      = -cur_val;
        }

        ascii_code_t digit_buffer [max_digits];
        auto end   = std::end(digit_buffer);
        auto begin = to_chars (end, cur_val);
        write_ptr = std::copy (begin, end, write_ptr); 
        
        if (buffer[1] != u8':')
        {
            *write_ptr++ = u8':';
        }

        *write_ptr++ = (brace_level == 0 ? u8'}' : u8']');
        out_proc.process_string ({buffer.data (), write_ptr});
        return true;
    }
    }
    return false;
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
constexpr auto control_text = ascii_code_t {0203};  /// control code for ‘@t’, ‘@^’, etc.
constexpr auto format       = ascii_code_t {0204};  /// control code for ‘@f’
constexpr auto definition   = ascii_code_t {0205};  /// control code for ‘@d’
constexpr auto begin_pascal = ascii_code_t {0206};  /// control code for ‘@p’
constexpr auto module_name  = ascii_code_t {0207};  /// control code for ‘@<’
constexpr auto new_module   = ascii_code_t {0210};  /// control code for ‘@ ’ and ‘@*’

// Declared in module 171 what needed here already
mod_pointer_t module_count;

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
name_pointer_t cur_module;
bool           scanning_hex = false;  /// are we scanning a hexadecimal constant

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
        id_first = loc;
        do
        {
            ++loc;
            d = buffer [loc];
        }
        while (is_alphanumeric (d) || d == u8'_');

        if (loc > id_first + 1)
        {
            c      = identifier;
            id_loc = loc;
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
    double_chars = 0_r;
    id_first     = loc - 1_r;

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

    id_loc = loc - 1_r;
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
            cur_module = prefix_lookup (k - 3);
        }
        else
        {
            cur_module = module_lookup (k);
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
        if (k < longest_name - 1)
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

    if (k > longest_name - 2)
    {
        err.terminal ().print_nl ("! Section name too long: ");
        for (auto j = inname_index_t {1}; j <= 25; ++j)
        {
            print (err.terminal (), mod_text [j]);
        }
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
    int            val = 0;
    name_pointer_t q;

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
        q = id_lookup (normal);
        if (ilk [q] != numeric)
        {
            next_control = u8'*';  // leads to error
            return scan_numeric_cases::reswitch;
        }

        accumulator += next_sign * (equiv [q] - 0100000);
        next_sign = 1_r;
        return scan_numeric_cases::consumed;

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
void
scan_numeric (name_pointer_t p)
{
    int                accumulator = 0;    /// accumulates sums
    int             next_sign   = 1;  /// sign to attach to next value

    scan_numeric_cases state;
    do
    {
        next_control = get_next ();
        do { state = scan_numeric_one (accumulator, next_sign); }
        while (state == scan_numeric_cases::reswitch);
    }
    while (state != scan_numeric_cases::done);

    if (std::abs (accumulator) >= 0100000)
    {
        err.err_print ("! Value too big: ", accumulator);
        accumulator = 0;
    }
    equiv [p] = accumulator + 0100000;  // name p now is defined to equal accumulator
}

// section 163 nothing tbd
// section 164

/// replacement text formed by scan_repl
auto cur_repl_text = text_pointer_t {};

// section 165, 167

void
copy_string_from_buffer_to_tok_mem ();
void
copy_verbatim_from_buffer_to_tok_mem ();
void
ensure_parantheses_balance (int &balance);

void
scan_repl (uint8_t type)
{
    int      balance = 0;  /// left parentheses minus right parentheses
    uint16_t a;

    bool     done = false;

    do
    {
        a = get_next ();

        switch (a)
        {
        case u8'(': ++balance; break;

        case u8')':
            if (balance == 0)
            {
                err.err_print ("! Extra )");
            }
            else
            {
                --balance;
            }
            break;

        case u8'\'': copy_string_from_buffer_to_tok_mem (); break;

        case u8'#':
            if (type == parametric)
            {
                a = param;
            }
            break;

        case identifier:
            a = id_lookup (normal);
            app_repl (0200 + (a >> 8));
            a &= 0xFF;
            break;

        case module_name:
            if (type == module_name)
            {
                app_repl (0250 + (cur_module >> 8));
                a = cur_module & 0xFF;
                break;
            }
            done = true;
            break;

        case verbatim: copy_verbatim_from_buffer_to_tok_mem (); break;

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
        }

        if (!done)
        {
            app_repl (a & 0xFF);  // store a in tok_mem
        }
    }
    while (!done);

    next_control = a & 0xFF;
    ensure_parantheses_balance (balance);
    if (text_ptr > max_texts - zz)
        err.overflow ("text");

    cur_repl_text             = text_ptr;
    tok_start [text_ptr + zz] = tok_ptr [z];
    ++text_ptr;
    if (z == zz - 1)
    {
        z = 0_r;
    }
    else
    {
        ++z;
    }
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
        app_repl (u8')');
        --balance;
    }
}

// section 168

void
copy_string_from_buffer_to_tok_mem ()
{
    ascii_code_t b = u8'\'';

    while (true)
    {
        app_repl (b);
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
            app_repl (u8'\'');
        }
    }
}

// section 169

void
copy_verbatim_from_buffer_to_tok_mem ()
{
    app_repl (verbatim);
    buffer [limit + 1_r] = u8'@';

    while (true)
    {
        if (buffer [loc] == u8'@')
        {
            if (loc < limit)
            {
                if (buffer [loc + 1_r] == u8'@')
                {
                    app_repl (u8'@');
                    loc += 2_r;
                    continue;
                }
            }
        }
        else
        {
            app_repl (buffer [loc++]);
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
}

// section 170

void
define_macro (ilk_value type)
{
    auto p = id_lookup (type);
    scan_repl (type);
    equiv [p]                 = cur_repl_text;
    text_link [cur_repl_text] = 0;
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
            scan_numeric (id_lookup (numeric));
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
    name_pointer_t p;
    switch (next_control)
    {
    case begin_pascal: p = 0_r; break;
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

    default: return;
    }

    // Insert module number into tok_mem
    store_two_bytes (0150000 + module_count);  // 01500000 = 0320 * 0400
    scan_repl (module_name);                   // now cur_repl_text points to the replacement text

    if (p == 0)  // unnamed module
    {
        text_link [last_unnamed] = cur_repl_text;
        last_unnamed             = cur_repl_text;
    }
    else if (equiv [p] == 0)  // first module of this name
    {
        equiv [p] = cur_repl_text;
    }
    else
    {
        p = name_pointer_t {equiv [p]};
        while (text_link [p] < module_flag)  // find end of list
        {
            p = name_pointer_t {text_link [p]};
        }
        text_link [p] = cur_repl_text;
    }

    text_link [cur_repl_text] = module_flag;  // mark this replacement text as nonmacro
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
    std::fill (byte_start.begin (), byte_start.begin () + ww + 1, 0_r);  // one more to make name 0 of
                                                                       // length 0
    std::fill (byte_ptr.begin (), byte_ptr.end (), 0_r);
    name_ptr       = 1_r;
    string_ptr     = 256_r;
    pool_check_sum = 271828;

    // section 46
    std::fill (tok_start.begin (), tok_start.begin () + zz + 1, 0_r);  // one more to make replacement text
                                                                     // 0 of length 0
    std::fill (tok_ptr.begin (), tok_ptr.end (), 0_r);
    text_ptr = 1_r;
    z        = 1_r;

    // section 48
    rlink [0_r] = 0;
    equiv [0_r] = 0;

    // section 52
    std::fill (hash.begin (), hash.end (), 0_r);
    std::fill (chop_hash.begin (), chop_hash.end (), 0_r);

    // section 71
    last_unnamed    = 0_r;
    text_link [0_r] = 0;

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

    out_buf.set_on_new_line (on_new_line);
    out_buf.set_on_line_truncated (on_line_truncated);
    out_proc.set_on_missing_sign_between_numbers (on_missing_sign_between_numbers);

    initialize ();
    initialize_input_system ();
    term.print_ln ("{}", banner);

    err.set_print_error_location (print_error_location_input);
    module_count = 0_r;

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

        char digit_buffer [max_digits];
        std::to_chars (digit_buffer, std::end(digit_buffer), pool_check_sum);
        for (size_t i = 0; i < 9; ++i)
        {
            write (pool, digit_buffer [i]);
        }
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
