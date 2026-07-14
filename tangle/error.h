#pragma once

#include "terminal.h"

using print_error_location_t = void (*) (terminal &);

class error_manager
{
    enum history_enum
    {
        spotless,
        harmless_message,
        error_message,
        fatal_message
    };

    history_enum           history              = spotless;
    print_error_location_t print_error_location = {};
    ::terminal              &term;

  public:
    error_manager (::terminal &term) : term (term) {}

    ::terminal &
    terminal () const { return term; }

    inline void
    set_print_error_location (print_error_location_t f)
    { print_error_location = f; }

    inline void
    mark_harmless ()
    {
        if (history == spotless)
        {
            history = harmless_message;
        }
    }

    inline void
    mark_error ()
    { history = error_message; }

    inline void
    mark_fatal ()
    { history = fatal_message; }

    inline int exit_code () { return history; }

    inline void
    error ()
    {
        print_error_location (term);
        term.update ();
        mark_error ();
    }

    template <typename... Args>
    void
    err_print (std::format_string<Args...> fmt, Args &&...args)
    {
        term.print_nl (fmt, std::forward<Args> (args)...);
        error ();
    }

    template <typename... Args>
    void
    fatal_error (std::format_string<Args...> fmt, Args &&...args)
    {
        err_print (fmt, std::forward<Args> (args)...);
        mark_fatal ();
        std::exit (history);
    }

    inline void
    confusion (std::string_view what)
    { fatal_error ("! This can't happen ({})", what); }

    inline void
    overflow (std::string_view what)
    { fatal_error ("! Sorry, {} capacity exceeded", what); }
};
