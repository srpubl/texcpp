#pragma once

#include <string_view>
#include <vector>

#include "pascal/text_file.h"

#include "character.h"

using on_new_line_t      = void (*) (int line);
using on_line_truncated_t = void (*) ();

/// Writes value backwards into a buffer starting from (the byte before) end
/// Returns the start of the string, anchored at the tail of the buffer
/// Caller must make sure that value is not negative and that buffer is large enough
constexpr ascii_code_t *
to_chars (ascii_code_t *end, int value) noexcept
{
    do
    {
        auto [quot, rem] = std::div (value, 10);
        *(--end)         = static_cast<ascii_code_t> (u8'0' + rem);
        value            = quot;
    }
    while (value > 0);

    return end;
}

inline void
write (pascal::text_file &file, ascii_code_t c)
{ file.write (convert_to_output (c)); }


class out_buffer
{
    std::vector<ascii_code_t> buffer = {};
    pascal::text_file        &pascal_file;
    size_t                    break_index      = 0; /// last breaking place in out_buf
    size_t                    semi_index       = 0; /// last semicolon breaking place in out_buf

    int                       line_length      = 0;
    int                       line             = 1;

    on_new_line_t             on_new_line      = nullptr;
    on_line_truncated_t       on_line_truncated = nullptr;

  public:
    out_buffer (int line_length, pascal::text_file &pascal_file)
        : line_length (line_length), pascal_file (pascal_file)
    { buffer.reserve (2 * line_length); }

    int
    current_line ()
    { return line; }

    void
    set_on_new_line (on_new_line_t f)
    { on_new_line = f; }

    void
    set_on_line_truncated (on_line_truncated_t f)
    { on_line_truncated = f; }

    void
    append (ascii_code_t ch)
    { buffer.push_back (ch); }

    void
    append_value (int value)
    {
        ascii_code_t digit_buffer [11];
        auto         end   = &digit_buffer [11];
        auto         begin = to_chars (end, value);
        buffer.insert (buffer.end (), begin, end);
    }

    void
    mark_break ()
    { break_index = buffer.size (); }

    void 
    mark_semicolon ()
    { semi_index = buffer.size (); }

    void
    flush_line ();

    void
    flush_line_if_too_long ()
    {
        if (buffer.size () > line_length)
        {
            flush_line ();
        }
    }

    void
    flush_all ()
    {
        while (!buffer.empty ())
        {
            if (buffer.size () <= line_length)
            {
                mark_break ();
            }
            flush_line ();
        }
    }

    void
    flush_last_line ()
    {
        mark_break ();
        semi_index  = 0;
        flush_line ();
    }

    /// Returns a u8string_view for the entire buffer
    /// This view is temporary, will not be updated, and should be discarded as
    /// soon as changes happen to the buffer
    std::u8string_view
    temporary_view () const
    { return {buffer.data (), buffer.size ()}; }

    /// Returns a u8string_view referring to the part after the last marked break
    /// This view is temporary, will not be updated, and should be discarded as
    /// soon as changes happen to the buffer
    std::u8string_view
    temporary_view_after_break () const
    { return {buffer.data () + break_index, buffer.size () - break_index}; }
};
