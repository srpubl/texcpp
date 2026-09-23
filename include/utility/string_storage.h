#pragma once

#include <cstdint>
#include <new>
#include <stdexcept>
#include <string_view>
#include "utility/static_vector.h"

namespace util
{

template <typename>
inline constexpr char const * descriptive_type_name = ""; 

template <typename Record_T, typename Index_T = uint32_t>
class string_storage 
{
public:
    using record_type = Record_T;
    using char_type = record_type::char_type;
    using string_view = std::basic_string_view <char_type>;
    using index_t = Index_T;

private:
    util::static_vector <char_type> chars = {};
    util::static_vector <record_type> records = {};

public:
    void
    initialize (size_t max_chars, size_t max_records)
    {
        chars.clear ();
        chars.reserve (max_chars + 1);

        records.clear ();
        records.reserve (max_records + 1);
        records.emplace_back (chars.data ());
        records.emplace_back (chars.data ()); // one more to make record 0 of length 0
        // records.resize cannot be used here because it would require Record_T to be CopyInsertable
    }

    auto &
    record_0 () { return *records.data(); }

    // TODO: remove once not needed anymore
    constexpr auto 
    index_of (record_type const &record) const -> index_t
    { return &record - records.data(); }

    auto const &
    record_at (index_t index) const
    { return records[index]; }

    constexpr auto &
    next_new () const
    { return records.back(); }

private:
    constexpr auto &
    next_new ()
    { return records.back (); }

    void
    emplace_record ()
    {
        try 
        {
            records.emplace_back (chars.data () + chars.size ());
        }
        catch (std::bad_alloc)
        {
            throw std::length_error (descriptive_type_name <record_type>);
        }
    }


public:
    void
    append_to_next_new (char_type c)
    {
        try
        {
            chars.push_back (c);
        }
        catch (std::bad_alloc)
        {
            throw std::length_error (descriptive_type_name <char_type>);
        }
    }

    void
    append_to_next_new (string_view str)
    {
        try
        {
            chars.insert (chars.end (), str.begin (), str.end ());
        }
        catch (std::bad_alloc)
        {
            throw std::length_error (descriptive_type_name <char_type>);
        }
    }

    record_type &
    add_next_new ()
    {
        auto &new_record = next_new ();
        emplace_record ();
        return new_record;
    }

    record_type &
    add (string_view id)
    {
        auto &new_record = next_new ();
        append_to_next_new (id);
        emplace_record ();
        return new_record;
    }

    // The last record that has actually been used.
    constexpr auto &
    last () const { return record_at (records.size () - 2); }

    void
    remove_last () 
    { 
        auto last_length = (++records.rbegin()) -> length();
        records.pop_back ();
        chars.resize (chars.size () - last_length); 
    }
};

}

