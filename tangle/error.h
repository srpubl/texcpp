#pragma once

#include "terminal.h"

using print_error_location_t = void (*) (terminal &);

struct error_state
{
    enum history_enum
    {
        spotless,
        harmless_message,
        error_message,
        fatal_message
    };

    history_enum  history = spotless;
    terminal              &term;

    error_state (terminal &term) : term (term)
    {}

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
};

class error_manager
{
    error_state &state;

  public:
    error_manager (error_state &state) : state (state) {}

    ::terminal &
    terminal () const { return state.term; }

    inline void
    error ()
    {
        print_error_location ();
        state.term.update ();
        state.mark_error ();
    }

    void mark_harmless () { state.mark_harmless(); }

    template <typename... Args>
    void
    err_print (std::format_string<Args...> fmt, Args &&...args)
    {
        terminal ().print_nl (fmt, std::forward<Args> (args)...);
        error ();
    }    

    template <typename... Args>
    void
    fatal_error (std::format_string<Args...> fmt, Args &&...args)
    {
        err_print (fmt, std::forward<Args> (args)...);
        state.mark_fatal ();
        std::exit (state.history);
    }

    inline void
    confusion (std::string_view what)
    { fatal_error ("! This can't happen ({})", what); }

    inline void
    overflow (std::string_view what)
    { fatal_error ("! Sorry, {} capacity exceeded", what); }

protected:
    virtual void 
    print_error_location () = 0;
};

