#include "output_token_stream.h"
#include "text_manager.h"
#include "tokens.h"

char32_t
output_token_stream::get_output ()
{
    while (true)  // because we need to restart once in a while
    {
        if (!has_more())
            return 0;

        if (!current_level_has_more())
        {
            _extra = -cur_state().module_number;
            pop_level ();
            if (_extra == 0)
                continue;

            return module_number;
        }

        auto a = get_char();

        if (a < 0x80)  // one-byte token
        {
            if (a != param)
                return a;

            // section 92
            // start scanning current macro parameter
            push_level (name_mgr.last ());
            continue;
        }

        if (a < 0xA800)
        {
            a -= 0x8000;
            auto &name = name_mgr.name_at(a);

            // section 89

            switch (name.ilk ())
            {
            case normal    : _extra = a; return identifier;
            case numeric   : _extra = name.number (); return number;
            case simple    : push_level (name); continue;
            case parametric: push_parametric (name); continue;
            default        : diagnose.on_invalid_ilk ();
            }
        }

        if (a < 0xD000)
        {
            // section 88

            a -= 0xA800;
            auto &name = name_mgr.name_at (a);
            if (name.replacement_text() != 0)
            {
                push_level (name);
            }
            else if (a != 0)
            {
                diagnose.on_name_not_found (name.content());
            }
            continue;
        }

        cur_state().module_number = _extra = a - 0xD000;
        return module_number;
    }
}

void
output_token_stream::push_parametric (name_t const &name)
{
    while (!current_level_has_more () && has_more()) { pop_level (); }

    if (!has_more () || peek_char () != u8'(')
    {   
        diagnose.on_missing_parameter (name.content ());
        return;
    }

    auto &new_text = copy_parameter_to_text_mgr ();
    new_text.set_continuation (&text_mgr.root ());
    name_mgr.add_simple (new_text);

    push_level (name);
}

text_t &
output_token_stream::copy_parameter_to_text_mgr ()
{
    auto builder = text_mgr.make_builder ();
    auto &str = cur_state().bytes;
    int balance = 1;  /// excess of ( versus ) while copying a parameter
    str.remove_prefix (1);  // opening (
    while (balance > 0)
    {
        auto b = str [0];
        switch (b)
        {
        case U'(': 
            ++balance; 
            str.remove_prefix (1);
            builder << b;
            break;

        case U')':
            str.remove_prefix (1);
            if (--balance == 0)
                break;

            builder << b;
            break;

        case U'\'':
        {
            auto slice = str.substr (0, str.find(U'\'', 1) + 1);   
            builder << slice;                    
            str.remove_prefix (slice.size());
            break;  
        }

        case param:
            str.remove_prefix (1);
            builder << (0x8000 + name_mgr.index_of (name_mgr.last ()));
            break;

        default:
            str.remove_prefix (1);
            builder << b;
            break;
        }            
    }

    return builder.finalize (); 
}

