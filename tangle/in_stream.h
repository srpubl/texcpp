#pragma once

#include "input_line_buffer.h"

class in_stream
{
public:
    struct diagnostics 
    {
        virtual void on_line_too_long () = 0;
    };

private:
    diagnostics &
    diag;

    input_line_buffer 
    _line;

    size_t 
    _loc;  /// the next character position to be read from the buffer

public:
    in_stream (diagnostics &diag, size_t max_line_size) : diag (diag), _line (max_line_size) {}

    auto &line () { return _line; }
    auto eol () const { return _loc > _line.limit (); }
    auto end_of_content () const { return _loc >= _line.limit (); }
    auto peek () const { return _line [_loc]; }
    auto get () { return _line [_loc++];}
    auto peek_back (size_t n = 1) const { return _line [_loc - n]; }
    auto peek_ahead (size_t n = 1) const { return _line [_loc + n]; }
    void advance (size_t n = 1) { _loc += n; }
    void retreat (size_t n = 1) { _loc -= n; }
    void seek (size_t pos) { _loc = pos; }

    void patch (std::u8string_view content) { _line.patch(_loc, content); }
    auto tell () const { return _loc; }

    auto read_line_from (pascal::text_file &file)
    {
        return _line.read_from (file, [this]() { diag.on_line_too_long(); });
    }
};

