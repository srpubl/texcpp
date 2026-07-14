#pragma once

// misleading name, ASCII is only 0..127
using ascii_code_t = char8_t;

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
