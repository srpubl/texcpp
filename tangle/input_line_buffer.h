#pragma once

#include <functional>
#include <string_view>
#include "utility/static_vector.h"
#include "pascal/text_file.h"

class input_line_buffer : private util::static_vector<char8_t>
{
    static constexpr size_t extra_chars = 2; 
    using char_type = char8_t;
    using base_type = util::static_vector <char_type>;
    using string_view = std::basic_string_view <char_type>;

public:
    input_line_buffer (size_t max_size) : base_type ()
    {
        reserve (max_size + extra_chars);
    }

    using base_type::operator[];

    auto limit () const { return size () - extra_chars; }
    auto up_to (size_t n) const { return string_view { data (), n};}
    auto after (size_t n) const { return string_view { begin () + n, begin () + limit ()}; }
    auto content () const { return up_to (limit ()); }
    void mark_end (char_type c) { back () = c; }
    void pad_end (char_type c) { (*this)[limit ()] = c; }
    void patch (size_t start, string_view content) { std::ranges::copy (content, begin () + start); }
    void clear () { resize (2); }
    void copy_content_from (input_line_buffer const &other) { assign (other.begin (), other.end ()); }

    void 
    set (string_view content)
    {
        resize (content.length () + extra_chars);
        assign (content.begin (), content.end ());
    }

    bool 
    matches (input_line_buffer const &other) 
    {
        if (limit () != other.limit ())
            return false;

        return content () == other.content();
    }


    // Reads a line from the given file into the buffer array, converting characters to their ASCII codes.
    // Returns true if a line was read, false if the end of the file was reached.
    bool
    read_from (pascal::text_file &file, std::function<void ()> on_line_too_long);
};

