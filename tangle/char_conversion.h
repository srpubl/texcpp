#pragma once

#include "character.h"
#include "terminal.h"

namespace detail
{
    inline constexpr auto xchr = std::array<text_char, 256> {
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
convert_to_output (ascii_code_t c) -> text_char
{ return detail::xchr [c]; }

namespace detail
{
    [[nodiscard]] inline constexpr auto
    generate_input_table ()
    {
        auto table = std::array<ascii_code_t, 256> {};
        for (size_t i = first_text_char; i <= last_text_char; ++i) { table [i] = u8' '; }
        for (size_t i = 1; i <= 255; ++i)
        {
            auto   c      = static_cast<ascii_code_t> (i);
            size_t index  = convert_to_output (c);
            table [index] = c;
        }
        table [' '] = u8' ';
        return table;
    }

    inline constexpr std::array<ascii_code_t, 256> xord = generate_input_table ();
}


[[nodiscard]] inline constexpr auto
convert_from_input (text_char c) -> ascii_code_t
{ return detail::xord [c]; }
