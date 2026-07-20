#include "out_buffer.h"

void
out_buffer::flush_line ()
{
    auto last_break_index = break_index;

    if (semi_index != 0 && buffer.size () - semi_index <= line_length)
    {
        break_index = semi_index;
    }

    for (auto k = buffer.begin (); k < buffer.begin () + break_index; ++k) { write (pascal_file, *k); }
    pascal_file.write_line ();
    
    ++line;
    if (on_new_line)
    {
        on_new_line (line);
    }

    if (break_index < buffer.size ())
    {
        if (buffer [break_index] == u8' ')
        {
            // drop space at break
            if (++break_index > last_break_index)
            {
                last_break_index = break_index;
            }
        }

        // shift remaining line to front
        buffer.erase (buffer.begin (), buffer.begin () + break_index);
    }
    else
    {
        buffer.clear ();
    }

    break_index = last_break_index - break_index;
    semi_index  = 0;

    if (buffer.size () > line_length)
    {
        if (on_line_truncated)
        {
            on_line_truncated ();
        }
        buffer.resize (line_length);
    }
}
