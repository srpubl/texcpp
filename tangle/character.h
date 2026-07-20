#pragma once

#include <algorithm>
#include <array>

#include "utility/between.h"

// misleading name, ASCII is only 0..127
using ascii_code_t = char8_t;  /// the internally used type

constexpr auto and_sign         = ascii_code_t {04};   // equivalent to 'and'
constexpr auto not_sign         = ascii_code_t {05};   // equivalent to 'not'
constexpr auto set_element_sign = ascii_code_t {06};   // equivalent to 'in'
constexpr auto tab_mark         = ascii_code_t {011};  // ASCII code used as tab-skip
constexpr auto line_feed        = ascii_code_t {012};  // ASCII code thrown away at end of line
constexpr auto form_feed        = ascii_code_t {014};  // ASCII code used at end of page
constexpr auto carriage_return  = ascii_code_t {015};  // ASCII code used at end of line
constexpr auto left_arrow       = ascii_code_t {030};  // equivalent to ':='
constexpr auto not_equal        = ascii_code_t {032};  // equivalent to '<>'
constexpr auto less_or_equal    = ascii_code_t {034};  // equivalent to '<='
constexpr auto greater_or_equal = ascii_code_t {035};  // equivalent to '>='
constexpr auto equivalence_sign = ascii_code_t {036};  // equivalent to '=='
constexpr auto or_sign          = ascii_code_t {037};  // equivalent to 'or'

constexpr bool
is_digit (ascii_code_t c)
{ return is_between (c, u8'0', u8'9'); }

constexpr bool
is_octal (ascii_code_t c)
{ return is_between (c, u8'0', u8'7'); }

constexpr bool
is_upper (ascii_code_t c)
{ return is_between (c, u8'A', u8'Z'); }

constexpr bool
is_lower (ascii_code_t c)
{ return is_between (c, u8'a', u8'z'); }

constexpr bool
is_alpha (ascii_code_t c)
{ return is_upper (c) || is_lower (c); }

constexpr bool
is_hex (ascii_code_t c)
{ return is_digit (c) || is_between (c, u8'A', u8'F'); }

constexpr bool
is_alphanumeric (ascii_code_t c)
{ return is_alpha (c) || is_digit (c); }


namespace detail
{
inline constexpr auto xchr = std::array<char, 256> {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  '!', '"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4',  '5', '6', '7', '8', '9', ':', ';',
    '<', '=', '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',  'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_', '`', 'a', 'b', 'c',
    'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',  'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', '{', '|', '}', '~', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' '  //
};
}

[[nodiscard]] inline constexpr auto
convert_to_output (ascii_code_t c) -> char
{ return detail::xchr [c]; }

namespace detail
{
[[nodiscard]] inline constexpr auto
generate_input_table ()
{
    auto table = std::array<ascii_code_t, 256> {};
    std::fill (table.begin (), table.end (), u8' ');
    for (ascii_code_t c = 1; c != 0; ++c)  // overflows into 0 after c == 255
    {
        table [convert_to_output (c)] = c;
    }
    table [' '] = u8' ';
    return table;
}

inline constexpr std::array<ascii_code_t, 256> xord = generate_input_table ();
}  // namespace detail

[[nodiscard]] inline constexpr auto
convert_from_input (char c) -> ascii_code_t
{ return detail::xord [c]; }
