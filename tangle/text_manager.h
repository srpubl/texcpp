#pragma once

#include "utility/string_storage.h"

#include "text.h"

class text_manager
{
public:
    using storage_type = util::string_storage <text_t>;
    using char_type = storage_type::char_type;
    using string_view = storage_type::string_view; 

private:
    storage_type storage;
    text_t * last_unnamed;

public:
    auto & root () { return storage.record_0 (); }

    void initialize (size_t max_tokens, size_t max_texts)
    {
        storage.initialize (max_tokens, max_texts);
        root ().set_continuation (&root ());
        last_unnamed = &root ();
    }


    void append_to_next_new (char_type b)   { storage.append_to_next_new (b); }
    void append_to_next_new (string_view b) { storage.append_to_next_new (b); }
    void remove_last ()    { storage.remove_last(); }
    auto & add_next_new () { return storage.add_next_new(); }

    void add_unnamed (text_t &text)
    {
        last_unnamed -> set_continuation (&text);
        last_unnamed = &text;
    }
};

