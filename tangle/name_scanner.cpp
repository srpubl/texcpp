#include "name_scanner.h"
#include "tokens.h"

char8_t
name_scanner::scan_identifier (patched_in_stream &in_str)
{
    char8_t c = in_str.peek_back ();
    if (in_str.tell () > 1
        && (c == u8'E' || c == u8'e')
        && is_digit (in_str.peek_back (2))
    )  
        return u8'E';

    char8_t d;
    auto id_first = in_str.tell () - 1;
    do
    {
        d = in_str.get ();
    }
    while (is_alphanumeric (d) || d == u8'_');
    in_str.retreat ();

    if (in_str.tell () > id_first + 1)
    {
        content = {& in_str.line () [id_first], static_cast<size_t> (in_str.tell () - id_first)};
        return identifier;
    }

    return c;
}

char8_t
name_scanner::scan_preprocessed_string (patched_in_stream &in_str)
{
    char8_t d;
    double_chars  = 0;
    auto id_first = in_str.tell () - 1;

    do
    {
        d = in_str.get ();
        if (d == u8'"' || d == u8'@')
        {
            if (in_str.peek () == d)
            {
                in_str.advance ();
                d = 0;
                ++double_chars;
            }
            else if (d == u8'@')
            {
                diagnose.on_missing_double_escape (in_str.err ());
            }
        }
        else if (in_str.eol ())
        {
            diagnose.on_string_without_end (in_str.err ());
            d = u8'"';
        }
    }
    while (d != u8'"');

    content = {& in_str.line () [id_first], static_cast<size_t> (in_str.tell () - 1 - id_first)};
    return identifier;
}

