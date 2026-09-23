#include "character.h"
#include "input_line_buffer.h"

bool
input_line_buffer::read_from (pascal::text_file &file, std::function<void ()> on_line_too_long)
{
    if (file.eof ())
    {
        resize (2);
        return false;
    }

    auto final_limit = size_t {};  /// limit without trailing blanks
    resize (0);

    while (!file.eol ())
    {
        push_back (convert_from_input (file.current ()));
        file.get ();

        if (back () != u8' ')
        {
            final_limit = size ();
        }

        // If input line is longer than buffer: discard all extra characters and signal error
        if (size () == capacity () - extra_chars + 1)
        {
            while (!file.eol ()) { file.get (); }
            pop_back ();
            final_limit = std::min (final_limit, size ());
            on_line_too_long ();
        }
    }

    file.read_line ();
    resize (final_limit + 2);
    return true;
}


