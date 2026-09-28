#pragma once

#include <cstddef>
#include <filesystem>

#include "change_stream.h"
#include "diagnostics/in_error_manager.h"
#include "in_stream.h"

class patched_in_stream
{
public:
    struct diagnostics
    {
        virtual ~diagnostics () = default;
        virtual void on_lines_dont_match (size_t non_matching_lines) = 0;
        virtual void on_web_file_ended_during_change () = 0;
    };

private:
    diagnostics &
    diagnose;

    in_stream 
    web_str;
    
    change_stream 
    change_str;

    in_stream 
    *active_str = &web_str;

public:
    patched_in_stream (
        diagnostics &diagnose,
        in_stream::diagnostics &web_str_diag, 
        change_stream::diagnostics &change_str_diag,
        size_t max_line_size)
        : diagnose (diagnose)
        , web_str (web_str_diag, max_line_size)
        , change_str (change_str_diag, max_line_size) 
    { 
        static_cast <in_error_manager &> (web_str.err ()).set_stream (&web_str);
        static_cast <in_error_manager &> (change_str.err ()).set_stream (&change_str);
    }

    auto &line () { return active_str -> line (); }
    auto line_number () { return active_str -> line_number (); }
    auto &err () { return active_str -> err (); }
    auto eol () const { return active_str -> eol (); }
    auto eof () const { return web_str.eof (); }
    auto end_of_content () const { return active_str -> end_of_content (); }
    auto peek () const { return active_str -> peek (); }
    auto get () { return active_str -> get ();}
    auto peek_back (size_t n = 1) const { return active_str -> peek_back (n); }
    auto peek_ahead (size_t n = 1) const { return active_str -> peek_ahead (n); }
    void advance (size_t n = 1) { active_str -> advance (n); }
    void retreat (size_t n = 1) { active_str -> retreat (n); }
    void seek (size_t pos) { active_str -> seek (pos); }

    void patch (std::u8string_view content) { active_str -> patch (content); }
    auto tell () const { return active_str -> tell (); }

    void
    open (std::filesystem::path web_file_name, std::filesystem::path change_file_name)
    {
        active_str = &web_str;

        web_str.open (web_file_name);
        change_str.open (change_file_name);

        change_str.seek_target_line ();
    }

    void
    close ()
    {
        change_str.check_if_processed_all_changes ();
        web_str.close ();
        change_str.close ();
    }

    auto &active () { return *active_str; }

    bool
    read_line ();

private:
    auto
    match_target_lines_and_choose_stream () -> ::in_stream *;
};
