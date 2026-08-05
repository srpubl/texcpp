#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

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
    std::vector <char_type> chars = {};
    std::vector <record_type> records = {};

public:
    void
    initialize (size_t max_chars, size_t max_records)
    {
        chars.clear ();
        chars.reserve (max_chars + 1);

        records.clear ();
        records.reserve (max_records + 1);
        records.resize (2, record_type {chars.data ()});  // one more to make record 0 of length 0
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

    constexpr void
    check_size_of_chars (index_t count) const
    {
        if (chars.size () + count > chars.capacity ())
            throw std::length_error (descriptive_type_name <char_type>);
    }

    constexpr void
    check_size_of_records () const
    {
        if (records.size () > records.capacity () - 1)
            throw std::length_error (descriptive_type_name <record_type>);
    }


public:
    void
    append_to_next_new (char_type c)
    {
        check_size_of_chars (1);
        chars.push_back (c);
    }

    void
    append_to_next_new (string_view str)
    {
        check_size_of_chars (str.length ());
        chars.insert (chars.end (), str.begin (), str.end ());
    }

    record_type &
    add_next_new ()
    {
        check_size_of_records();

        auto &new_record = next_new ();
        records.emplace_back (chars.data () + chars.size ());
        return new_record;
    }

    record_type &
    add (string_view id)
    {
        check_size_of_chars (id.length ());
        check_size_of_records();

        auto &new_record = next_new ();

        chars.insert (chars.end (), id.begin (), id.end ());
        records.emplace_back (chars.data () + chars.size ());

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

