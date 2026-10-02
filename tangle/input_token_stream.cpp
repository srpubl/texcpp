#include "input_token_stream.h"
#include "tokens.h"

char8_t
input_token_stream::control_code (char8_t c)
{
    switch (c)
    {
    case u8'@'   : return u8'@';
    case u8'\''  : return octal;
    case u8'"'   : return hex;
    case u8'$'   : return check_sum;
    case u8' '   :
    case tab_mark: return new_module;

    case u8'*':
        _diagnose.on_new_major_section ();
        return new_module;

    case u8'D':
    case u8'd' : return definition;

    case u8'F' :
    case u8'f' : return format;

    case u8'{' : return begin_comment;
    case u8'}' : return end_comment;

    case u8'P' :
    case u8'p' : return begin_pascal;

    case u8':' :
    case u8'T' :
    case u8't' :
    case u8'^' :
    case u8'.' : return control_text;

    case u8'&' : return join;
    case u8'<' : return module_name;
    case u8'=' : return verbatim;
    case u8'\\': return force_line;

    default    : return ignore;
    }
}

// Skips all characters until the next @ or eof
// Returns the control code if one was found
char8_t
input_token_stream::get_next_control_code ()
{
    while (true)
    {
        if (_in_str.eol () && !_in_str.read_line ())
            return new_module;

        // Put @ as marker so we don't have to check also for the end
        _in_str.line ().mark_end (u8'@');
        while (_in_str.get () != u8'@') {}  // find the next marker

        if (_in_str.eol ()) // found only the end marker
            continue;

        auto ascii = _in_str.get ();
        auto c     = control_code (ascii);
        if (c != ignore || ascii == u8'>')
            return c;
    }
}

/// Skips to next unmatched '}'
void
input_token_stream::skip_comment ()
{
    int balance = 0;
    while (true)
    {
        if (_in_str.eol () && !_in_str.read_line ())
        {
            _diagnose.on_input_end_in_comment (_in_str.err ());
            return;
        }

        switch (_in_str.get()) 
        {
        case u8'@':
            switch (_in_str.get ())
            {
            case u8' ':
            case tab_mark:
            case u8'*':
                _diagnose.on_section_end_in_comment (_in_str.err ());
                _in_str.retreat (2);
                return;
            }
            break;
        
        case u8'\\': 
            if (_in_str.peek () != u8'@')
            {
                _in_str.advance ();
            }
            break;
        
        case u8'{':
            ++balance;
            break;

        case u8'}':
            if (balance-- == 0)
                return;
            break;
        }
    }
}

char8_t
input_token_stream::get ()
{
    while (true)
    {
        if (_in_str.eol () && !_in_str.read_line ())
            return new_module;

        auto c = _in_str.get ();

        if (_scanning_hex)
        {
            if (is_hex (c))
                return c;

            _scanning_hex = false;
        }

        if (is_alpha (c))
            return _name_scnr.scan_identifier (_in_str);

        switch (c)
        {
        case u8'"': 
            return _name_scnr.scan_preprocessed_string (_in_str);

        case u8'@':
            c = control_code (_in_str.get ());
            switch (c)
            {
            case module_name:
                _mod_name_scnr.scan_module_name (_in_str);
                return module_name;

            case hex:
                _scanning_hex = true;
                return hex;

            default:
                return c;

            case control_text:
                while (get_next_control_code () == u8'@') {}

                if (_in_str.peek_back () != u8'>')
                {
                    _diagnose.on_improper_marker (_in_str.err ());
                }
                continue;
                
            case ignore:
                continue;            
            }

        // section 147
        case u8'.': compress_if (c, '.', double_dot) || compress_if (c, ')', u8']'); return c;
        case u8':': compress_if (c, '=', left_arrow); return c;
        case u8'=': compress_if (c, '=', equivalence_sign); return c;
        case u8'>': compress_if (c, '=', greater_or_equal); return c;
        case u8'<': compress_if (c, '=', less_or_equal) || compress_if (c, '>', not_equal); return c;
        case u8'(': compress_if (c, '*', begin_comment) || compress_if (c, '.', u8'['); return c;
        case u8'*': compress_if (c, ')', end_comment); return c;
        case u8' ':
        case tab_mark: continue;
        case u8'{': skip_comment (); continue;
        case u8'}': _diagnose.on_extra_brace (_in_str.err ()); continue;

        default:
            if (c < 128)
                return c;
        }
    }
}

