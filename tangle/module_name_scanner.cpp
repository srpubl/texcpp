#include "module_name_scanner.h"
#include "patched_in_stream.h"

void 
module_name_scanner::read_module_name (patched_in_stream &in_str) 
{
    mod_name.resize (0);
    while (true)
    {
        if (in_str.eol ())
        {
            if (!in_str.read_line ())
            {
                diagnose.on_input_ended (in_str.err ());
                return;
            }
        }

        char8_t d = in_str.get ();
        if (d == u8'@')
        {
            d = in_str.get ();
            switch (d) 
            {
            case u8'>': 
                return;
            
            case u8' ':
            case tab_mark:
            case u8'*':
                in_str.retreat (2);
                diagnose.on_missing_end (in_str.err ());
                return;
            
            default:
                mod_name.push_back (u8'@');
            }
        }

        if (d == u8' ' || d == tab_mark)
        {
            if (mod_name.back () == u8' ')
                continue;

            d = u8' ';
        }

        if (mod_name.size () < config::longest_name - 1)
        {
            mod_name.push_back (d);
        }
    }
}


