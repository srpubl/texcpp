#include <string_view>

#include "pascal/array.h"
#include "pascal/range.h"

#include "name_manager.h"
#include "text_manager.h"


// TODO: Move to right place
constexpr auto param         = char8_t {0x00};

namespace internal
{

struct output_state
{
    text_manager::string_view bytes;
    name_t const * name;            /// pointer to current name being expanded
    text_t const * replacement;     /// pointer to current replacement text
    int  module_number;             /// module number or zero if not a module

    void initialize (text_t const * replacement)
    {
        name    = nullptr;
        set_replacement (replacement);
        module_number = 0;
    }

    void push (name_t const &name) 
    {
        this -> name = &name;
        set_replacement (name.replacement_text());
        module_number = 0;
    }

    void set_replacement (text_t const *replacement)
    {
        this -> replacement = replacement;
        bytes = replacement ? replacement -> content() : decltype(bytes) {};
    }
};

}

// section 79

constexpr char32_t number        = 0x80;  /// code returned by get_output when next output is numeric
constexpr char32_t module_number = 0x81;  /// code returned by get_output for module numbers
constexpr char32_t identifier    = 0x82;  /// code returned by get_output for identifiers

class output_token_stream 
{
public:
    struct error_handlers 
    {
        virtual void on_missing_parameter (std::u8string_view) = 0;
        virtual void on_name_not_found (std::u8string_view) = 0;
        virtual void on_stack_overflow () = 0;
        virtual void on_invalid_ilk () = 0;
    };

private:
    name_manager &name_mgr;
    text_manager &text_mgr;
    error_handlers &err;

    pascal::int_range_array<1, config::stack_size, internal::output_state>
    stack = {};
    
    pascal::int_range<0, config::stack_size> 
    stack_ptr = {};

    int _extra;

    auto & cur_state () { return stack [stack_ptr]; }

public:
    output_token_stream (name_manager &name_mgr, text_manager &text_mgr, error_handlers &err)
    : name_mgr (name_mgr), text_mgr (text_mgr), err (err)
    {}

    void
    initialize ()
    {
        using namespace pascal;
        stack_ptr = 1_r;
        cur_state().initialize (text_mgr.storage.record_0 ().continuation ());
    }

    char32_t
    get_output ();

    bool
    has_more () { return stack_ptr > 0; }

    /// additional information corresponding to output token
    int
    extra () { return _extra; }

private:
    void
    push_level (name_t const &name)
    {
        if (stack_ptr == config::stack_size)
            err.on_stack_overflow ();

        stack_ptr++;
        cur_state().push (name);
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

        --stack_ptr;  // go down to previous level
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
