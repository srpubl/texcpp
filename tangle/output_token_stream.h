#pragma once

#include <string_view>
#include <vector>

#include "config.h"
#include "name_manager.h"
#include "text_manager.h"

namespace internal
{

struct output_state
{
    name_t const * name;            /// pointer to current name being expanded
    int  module_number;             /// module number or zero if not a module
    text_t const * replacement;     /// pointer to current replacement text
    text_manager::string_view bytes;

    output_state (name_t const &name)
    : name (&name), module_number (0)
    {
        set_replacement (name.replacement_text());
    }

    output_state (text_t const * replacement)
    : name (nullptr), module_number (0)
    {
        set_replacement (replacement);
    }

    void set_replacement (text_t const *replacement)
    {
        this -> replacement = replacement;
        bytes = replacement ? replacement -> content() : decltype(bytes) {};
    }
};

}

constexpr auto number        = 0x80;  /// code returned by get_output when next output is numeric
constexpr auto module_number = 0x81;  /// code returned by get_output for module numbers
constexpr auto identifier    = 0x82;  /// code returned by get_output for identifiers

class output_token_stream 
{
public:
    struct diagnostics 
    {
        virtual void on_missing_parameter (std::u8string_view) = 0;
        virtual void on_name_not_found (std::u8string_view) = 0;
        virtual void on_stack_overflow () = 0;
        virtual void on_invalid_ilk () = 0;
    };

private:
    name_manager &name_mgr;
    text_manager &text_mgr;
    diagnostics &diagnose;

    std::vector <internal::output_state>
    stack;

    int _extra;

    auto & cur_state () { return stack.back(); }

public:
    output_token_stream (name_manager &name_mgr, text_manager &text_mgr, diagnostics &diagnose)
    : name_mgr (name_mgr), text_mgr (text_mgr), diagnose (diagnose)
    {
        stack.reserve (config::stack_size);
    }

    void
    initialize ()
    {
        stack.emplace_back (text_mgr.storage.record_0 ().continuation ());
    }

    char32_t
    get_output ();

    bool
    has_more () { return stack.size() > 0; }

    /// additional information corresponding to output token
    int
    extra () { return _extra; }

    std::u8string_view
    extra_identifier () { return name_mgr.name_at (_extra).content(); }

private:
    void
    push_level (name_t const &name)
    {
        if (stack.size () == stack.capacity())
            diagnose.on_stack_overflow();

        stack.emplace_back (name);
    }

    void
    pop_level ()
    {
        auto continuation = cur_state().replacement -> continuation ();
        if (continuation == &text_mgr.storage.record_0 ())  // end of macro expansion
        {
            if (cur_state().name -> ilk() == parametric)
            {
                // pop parameter stack
                name_mgr.remove_last();
                text_mgr.storage.remove_last();
            }
        }
        else if (continuation)
        {
            cur_state().set_replacement (continuation); // stay on same level
            return;
        }

        stack.pop_back ();
    }

    bool
    current_level_has_more () { return ! cur_state ().bytes.empty (); }

    auto
    peek_char () { return cur_state().bytes[0]; }

    auto 
    get_char () 
    {
        auto a = peek_char (); 
        cur_state().bytes.remove_prefix(1);
        return a;
    }

    void
    push_parametric (name_t const &name);

    void
    copy_parameter_to_text_mgr ();
};
