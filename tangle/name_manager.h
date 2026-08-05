#pragma once

#include <array>
#include <string_view>

#include "config.h"

#include "utility/hash_bucket.h"
#include "utility/string_storage.h"
#include "name.h"

using namespace std::literals;

using index_t = uint32_t;

using on_error_t      = void (*) ();
using on_error_id_t   = void (*) (std::u8string_view id);
using on_add_string_t = index_t (*) (std::u8string_view id);

using hash_bucket_name_t_link = util::hash_bucket<name_t, &name_t::link, &name_t::set_link>;
using hash_bucket_name_t_chop_link = util::hash_bucket<name_t, &name_t::chop_link, &name_t::set_chop_link>;

template <>
inline constexpr char const * util::descriptive_type_name<name_t> = "name"; 

template <>
inline constexpr char const * util::descriptive_type_name<char8_t> = "byte memory"; 

class name_manager
{
    util::string_storage <name_t> storage;
    std::array<hash_bucket_name_t_link, config::hash_size> hash_bucket = {};
    std::array<hash_bucket_name_t_chop_link, config::hash_size> chop_hash_bucket = {};
    name_t *root = nullptr;

    on_error_t on_already_appeared = nullptr;
    on_error_t on_defined_before = nullptr;
    on_error_t on_incompatible = nullptr;
    on_error_t on_no_match = nullptr;
    on_error_t on_too_many_matches = nullptr;
    on_error_id_t on_id_conflict = nullptr;
    on_add_string_t on_add_string = nullptr;

public:
    void set_on_already_appeared (on_error_t f) { on_already_appeared = f; }
    void set_on_defined_before (on_error_t f) { on_defined_before = f; }
    void set_on_incompatible (on_error_t f) { on_incompatible = f; }
    void set_on_no_match (on_error_t f) { on_no_match = f; }
    void set_on_too_many_matches (on_error_t f) { on_too_many_matches = f; }
    void set_on_id_conflict (on_error_id_t f) { on_id_conflict = f; }

    void set_on_add_string (on_add_string_t f) { on_add_string = f; }

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
    add_simple (text_t * replacement_text)
    {
        auto &new_name = storage.add(u8""sv);
        new_name.set_ilk(simple);
        new_name.set_replacement_text (replacement_text);
    }

    constexpr auto 
    index_of (name_t const &name) const -> index_t
    { return storage.index_of(name); }

    auto const &
    name_at (index_t index) const
    { return storage.record_at(index); }

    constexpr auto
    index_of_next_new () const { return storage.index_of(storage.next_new ()); }

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
