#pragma once

#include "in_stream.h"

class change_stream : public in_stream  
{
public:
    using in_stream::in_stream;    

    struct diagnostics : public virtual in_stream::diagnostics
    {
        virtual void on_missing_x () = 0;
        virtual void on_missing_y () = 0;
        virtual void on_missing_z () = 0;
        virtual void on_ended_after_x () = 0;
        virtual void on_ended_without_z () = 0;
        virtual void on_extra_change () = 0;
    };

private:
    auto & diagnose () { return dynamic_cast <diagnostics &> (in_stream::diagnose); }

    char8_t
    get_change_control_letter ()
    {
        if (line ().limit () < 2 || line () [0] != u8'@')
            return 0;

        auto &c = line () [1];
        if (is_between (c, u8'X', u8'Z'))
        {
            c += (u8'z' - u8'Z');
        }
        return c;
    }

    bool
    skip_to_start_of_change ()
    {
        while (true)
        {
            if (!read_line ())
                return false;

            switch (get_change_control_letter ())
            {
            case u8'x': return true;
            case u8'y':
            case u8'z': seek(2); diagnose ().on_missing_x ();
            }
        }
    }

    void
    skip_blank_lines ()
    {
        do
        {
            if (!read_line ())
            {
                diagnose ().on_ended_after_x ();
                break;
            }
        }
        while (line ().empty ());
    }

public:
    void
    read_next_target_line ()
    {
        if (!skip_to_start_of_change ())
            return;

        skip_blank_lines ();
    }

    bool
    is_starting_replacement ()
    {
        switch (get_change_control_letter ())
        {
        case u8'y': return true;
        case u8'x':
        case u8'z': seek (2); diagnose ().on_missing_y ();
        }
        return false;
    }

    bool
    read_replacement_line ()
    {
        if (!read_line ())
        {
            diagnose ().on_ended_without_z ();
            line ().set (u8"@z");        
        }

        switch (get_change_control_letter ())
        {
        case u8'z': read_next_target_line (); return false;
        case u8'x':
        case u8'y': seek (2); diagnose ().on_missing_z ();
        }

        return true;
    }

    void
    check_if_processed_all_changes ()
    {
        if (!line ().empty ())
        {
            seek (line ().limit ());
            diagnose ().on_extra_change ();
        }
    }
};
