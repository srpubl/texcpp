#include "output_token_stream.h"

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

    copy_parameter_to_text_mgr ();

    auto &new_text = text_mgr.storage.add_next_new ();
    new_text.set_continuation (&text_mgr.storage.record_0 ());
    name_mgr.add_simple (&new_text);

    push_level (name);
}

void
output_token_stream::copy_parameter_to_text_mgr ()
{
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
            text_mgr.append_to_next_new (b);
            break;

        case U')':
            str.remove_prefix (1);
            if (--balance == 0)
                break;

            text_mgr.append_to_next_new (b);
            break;

        case U'\'':
        {
            auto slice = str.substr (0, str.find(U'\'', 1) + 1);   
            text_mgr.append_to_next_new (slice);                    
            str.remove_prefix (slice.size());
            break;  
        }

        case param:
            str.remove_prefix (1);
            text_mgr.append_to_next_new (0x8000 + name_mgr.index_of (name_mgr.last ()));
            break;

        default:
            str.remove_prefix (1);
            text_mgr.append_to_next_new (b);
            break;
        }            
    }
}

