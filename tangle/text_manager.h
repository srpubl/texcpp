#pragma once

#include "utility/string_storage.h"

#include "text.h"

class text_manager : private util::string_storage <text_t>
{
public:
    using storage_type = util::string_storage <text_t>;
    using storage_type::char_type;
    using storage_type::string_view;
    using storage_type::builder; 

private:
    text_t * last_unnamed;

public:
    auto & root () { return record_0 (); }

    void initialize (size_t max_tokens, size_t max_texts)
    {
        storage_type::initialize (max_tokens, max_texts);
        root ().set_continuation (&root ());
        last_unnamed = &root ();
    }

    using storage_type::make_builder;
    using storage_type::remove_last;

    void add_unnamed (text_t &text)
    {
        last_unnamed -> set_continuation (&text);
        last_unnamed = &text;
    }
};

