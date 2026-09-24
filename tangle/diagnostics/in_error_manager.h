#pragma once

#include "error.h"
#include "in_stream.h"

class in_error_manager : public error_manager
{
    in_stream *in_str = nullptr;

public:
    using error_manager::error_manager;

    void set_stream (in_stream *str) { in_str = str; } 

protected:
    virtual void print_intro () = 0;

    void print_error_location () override
    {        
        print_intro ();    
        terminal ().print_ln ("l.{})", in_str -> line_number ());

        // print characters already read
        auto index = std::min (in_str -> tell (), in_str -> line ().limit ());
        for (auto ch : in_str -> line().up_to (index))
        {
            terminal ().print (ch == tab_mark ? u8' ' : ch);
        }
        terminal ().print_nl ("{:>{}}", "", index);

        // print not yet read characters
        terminal ().print (in_str -> line().after (index));
        terminal ().print (' ');
    }
};

struct in_web_error_manager : public in_error_manager
{
    using in_error_manager::in_error_manager;
    void print_intro () override { terminal ().print (". ("); }
};

struct in_change_error_manager : public in_error_manager
{
    using in_error_manager::in_error_manager;
    void print_intro () override { terminal ().print (". (change file "); }
};


