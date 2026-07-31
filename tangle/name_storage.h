#pragma once

#include <vector>

#include "name.h"

class name_storage 
{
public:
    std::vector<char8_t> chars = {};
    std::vector<name_t> names = {};

public:
    void
    initialize (size_t max_chars, size_t max_names)
    {
        chars.clear ();
        chars.reserve (max_chars + 1);

        names.clear ();
        names.reserve (max_names + 1);
        names.resize (2, {chars.data (), names.data()});  // one more to make name 0 of length 0
    }

    auto &
    name_0 () { return *names.data(); }

    // TODO: remove once not needed anymore
    constexpr auto 
    index_of (name_t const &name) const -> index_t
    { return &name - names.data(); }

    auto const &
    name_at (index_t index) const
    { return names[index]; }

    constexpr auto &
    next_new ()
    { return *names.rbegin (); }

    constexpr auto &
    next_new () const
    { return *names.rbegin (); }

    constexpr auto
    is_next_new (name_t const &name) const
    { return &name == &next_new(); }

    void
    add (std::u8string_view id)
    {
        if (chars.size () + id.length () > chars.capacity ())
            throw std::length_error ("byte memory");

        if (names.size () > names.capacity () - 1)
            throw std::length_error ("name");

        chars.insert (chars.end (), id.begin (), id.end ());
        names.emplace_back (chars.data () + chars.size (), names.data());
    }

    // The last name that has actually been used.
    constexpr auto &
    last () const { return name_at (names.size () - 2); }

    void
    remove_last () { names.pop_back(); }
};

