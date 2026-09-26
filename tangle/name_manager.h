#pragma once

#include <array>
#include <functional>
#include <string_view>

#include "config.h"

#include "utility/hash_bucket.h"
#include "utility/string_storage.h"
#include "name.h"

using on_add_string_t = std::function <config::index_t ()>;

using hash_bucket_name_t_link = util::hash_bucket<name_t, &name_t::link, &name_t::set_link>;
using hash_bucket_name_t_chop_link = util::hash_bucket<name_t, &name_t::chop_link, &name_t::set_chop_link>;

template <>
inline constexpr char const * util::descriptive_type_name<name_t> = "name"; 

template <>
inline constexpr char const * util::descriptive_type_name<char8_t> = "byte memory"; 

class name_manager
{
public:
    struct diagnostics 
    {
        virtual void on_already_appeared () = 0;
        virtual void on_defined_before   () = 0;
        virtual void on_incompatible     () = 0;
        virtual void on_no_match         () = 0;
        virtual void on_too_many_matches () = 0;
        virtual void on_id_conflict      (std::u8string_view) = 0;

        virtual ~diagnostics () = default;
    };

private:
    diagnostics &diagnose;
    on_add_string_t on_add_string;

    util::string_storage <name_t> storage;
    std::array<hash_bucket_name_t_link, config::hash_size> hash_bucket = {};
    std::array<hash_bucket_name_t_chop_link, config::hash_size> chop_hash_bucket = {};
    name_t *root = nullptr;

public:
    name_manager (diagnostics &diagnose, on_add_string_t on_add_string) 
    : diagnose (diagnose), on_add_string (on_add_string) {}

    void
    initialize (size_t max_chars, size_t max_names);

    /// Finds current identifier if it exists or stores it.
    auto
    lookup (ilk_value t, std::u8string_view id) -> name_t &;

    auto
    lookup_module (std::u8string_view module_name) -> name_t &;

    auto
    lookup_prefix (std::u8string_view module_name) -> name_t &;

    void
    add_simple (text_t &replacement_text)
    {
        using namespace std::literals;
        auto &new_name = storage.add(u8""sv);
        new_name.set_ilk(simple);
        new_name.set_replacement_text (replacement_text);
    }

    constexpr auto 
    index_of (name_t const &name) const
    { return storage.index_of(name); }

    auto const &
    name_at (config::index_t index) const
    { return storage.record_at(index); }

    constexpr auto &
    no_name () { return storage.record_0 (); }

    constexpr auto &
    last () const { return storage.last (); }

    void
    remove_last () { storage.remove_last (); }

private:
    void
    update_name (name_t &p, ilk_value t, std::u8string_view id);

    void
    double_definition_error (name_t &p, ilk_value t);

    void
    update_secondary_hash (name_t &p);
    
    void
    remove_from_secondary_hash_table (name_t &name);    
};
