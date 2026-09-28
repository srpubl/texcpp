#include "patched_in_stream.h"

bool
patched_in_stream::read_line ()
{
    while (true)
    {
        if (active_str == &change_str)
        {
            if (change_str.read_replacement_line ())
                break;            

            active_str = &web_str;
        }

        if (!web_str.read_line ())
            break;

        if (change_str.line ().empty ())
            break;
        
        active_str = match_target_lines_and_choose_stream ();
        if (active_str == &web_str)
            break;
    }

    active_str -> seek (0);
    active_str -> line ().pad_end (u8' ');
    return !web_str.eof ();
}

auto
patched_in_stream::match_target_lines_and_choose_stream () -> ::in_stream *
{
    if (!web_str.line ().matches (change_str.line ()))
        return &web_str;

    size_t non_matching_lines = 0;

    while (true)
    {
        if (!change_str.read_target_line ())
            return &web_str;

        if (change_str.is_starting_replacement ())
        {
            if (non_matching_lines > 0)
            {
                change_str.seek (2);
                diagnose.on_lines_dont_match (non_matching_lines);
            }

            return &change_str;
        }

        if (!web_str.read_line ())
        {
            diagnose.on_web_file_ended_during_change ();
            return &web_str;
        }

        if (!web_str.line ().matches (change_str.line ()))
        {
            ++non_matching_lines;
        }
    }
}
