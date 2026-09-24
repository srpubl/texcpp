#pragma once

#include <cstddef>
#include "error.h"
#include "input_line_buffer.h"

class in_stream
{
public:
    struct diagnostics 
    {
        error_manager &err;

        diagnostics (error_manager &err) : err (err) {} 

        virtual void on_line_too_long () = 0;
    };

private:
    diagnostics &
    _diag;

    input_line_buffer 
    _line;

    pascal::text_file
    _file;

    size_t 
    _loc = 0;  /// the next character position to be read from the buffer

    size_t
    _line_number = 0;

public:
    in_stream (diagnostics &diag, size_t max_line_size) : _diag (diag), _line (max_line_size) {}

    auto &line () { return _line; }
    auto line_number () { return _line_number; }
    auto &err () { return _diag.err; }
    auto eol () const { return _loc > _line.limit (); }
    auto eof () const { return _file.eof(); }
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

    auto read_line ()
    {
        ++_line_number;
        return _line.read_from (_file, [this]() { _diag.on_line_too_long(); });
    }

    void open (std::filesystem::path web_file_name) 
    {
        _file.assign (web_file_name);
        _file.reset ();
    }

    void close () { _file.close (); }
};

