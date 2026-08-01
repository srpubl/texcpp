#include "name_manager.h"

#include "config.h"
#include <string_view>

using chopped_id_t = std::array<char8_t, unambig_length + 1>;

static auto
compute_hash_code (std::u8string_view id) -> index_t
{
    auto h = id [0];
    id.remove_prefix (1);
    for (auto c : id) { h = (h + h + c) % hash_size; };
    return h;
}

static auto
chop_id (std::u8string_view id, chopped_id_t &chopped_id)
{
    index_t length = 0;
    for (auto ch : id)
    {
        if (length == unambig_length)
            break;

        if (ch == u8'_')
            continue;

        if (ch >= u8'a')
        {
            ch -= 040;
        }

        chopped_id [length++] = ch;
    }
    return std::u8string_view {chopped_id.data(), length};
}

void
name_manager::initialize (size_t max_chars, size_t max_names)
{
    storage.initialize(max_chars, max_names);
    std::fill (hash_bucket.begin (), hash_bucket.end (), hash_bucket_name_t_link{});
    std::fill (chop_hash_bucket.begin (), chop_hash_bucket.end (), hash_bucket_name_t_chop_link {});
}

name_t &
name_manager::lookup (ilk_value ilk, std::u8string_view id)
{
    auto  h    = compute_hash_code (id);
    auto &name = compute_name_location (h, id);

    if (storage.is_next_new (name))
    {
        storage.add (id);

        if (id [0] == u8'"')
        {
            name.set_ilk (numeric);
            name.set_number (on_add_string (id));
        }
        else
        {
            if (ilk == normal)
            {
                update_secondary_hash (name);
            }
            name.set_ilk (ilk);
        }
    } 
    else if (ilk != normal)
    {
        double_definition_error (name, ilk);
        
        // the second definition wins: we force a new ilk on p
        name.set_ilk (ilk);
    }

    return name;
}

auto
name_manager::compute_name_location (index_t hash, std::u8string_view id) -> name_t &
{
    auto &bucket = hash_bucket [hash];
    auto name = bucket.find([id](name_t &p){ return p.content () == id; });

    if (name)
        return *name;

    bucket.prepend(storage.next_new());
    return storage.next_new();
}

void
name_manager::update_secondary_hash (name_t &name)
{
    auto buf        = chopped_id_t {};
    auto chopped_id = chop_id (name.content(), buf);
    auto hash       = compute_hash_code (chopped_id);
    auto &bucket    = chop_hash_bucket [hash];

    if (on_id_conflict)
    {
        bucket.for_each([this, chopped_id](name_t &old_name)
        {
            chopped_id_t buf;
            auto id = old_name.content ();
            if (chop_id (id, buf) == chopped_id)
            {
                on_id_conflict (id);
            }
        });
    }

    bucket.prepend(name);
}


void
name_manager::remove_from_secondary_hash_table (name_t &name)
{
    chopped_id_t chopped_id_buf = {};
    auto chopped_id = chop_id (name.content(), chopped_id_buf);
    auto hash = compute_hash_code (chopped_id);

    chop_hash_bucket [hash].remove (name);
}

void
name_manager::double_definition_error (name_t &name, ilk_value ilk)
{
    if (name.ilk () == normal)  // We have seen p before it was used
    {
        if (ilk == numeric)  // We don't allow numeric macros to be defined after their first use
        {
            if (on_already_appeared) { on_already_appeared (); }

            // nevertheless we treat it as numeric from now on
            // numeric macros are not stored in secondary hash table
            remove_from_secondary_hash_table (name);
        }

        // We only make it a message for numeric because it might break the internal math
        // All other cases are not problematic
    }
    else
    {
        if (on_defined_before) { on_defined_before (); }
    }
}

enum comparison_result
{
    less,
    equal,
    greater,
    prefix,
    extension
};

auto
compare_module_names (std::u8string_view new_name, std::u8string_view old_name) -> comparison_result
{
    size_t i        = 0;
    size_t len      = std::min (old_name.length (), new_name.length ());

    for (; i < len; ++i)
    {
        auto result = new_name [i] <=> old_name [i];
        if (result != std::strong_ordering::equal)
            return result == std::strong_ordering::less ? less : greater;
    }

    return i != old_name.length () ? prefix : i != new_name.length () ? extension : equal;
}

auto
name_manager::lookup_module (std::u8string_view module_name) -> name_t &
{
    auto c = greater;
    auto q = &storage.name_0();
    auto p = storage.name_0().rlink();

    while (p != &storage.name_0())
    {
        c = compare_module_names (module_name, p -> content());
        q = p;
        if (c == less)
        {
            p = q -> llink ();
        }
        else if (c == greater)
        {
            p = q -> rlink ();
        }
        else
        {
            if (c != equal)
            {
                if (on_incompatible) { on_incompatible (); }
                return storage.name_0();
            }
            return *p;
        }
    }

    auto &new_name = storage.next_new ();
    
    storage.add(module_name);

    if (c == less)
    {
        q -> set_llink (&new_name);
    }
    else
    {
        q -> set_rlink (&new_name);
    }

    return new_name;
}

auto
name_manager::lookup_prefix (std::u8string_view module_name) -> name_t &
{
    auto resume_node  = &storage.name_0();
    auto current_node = storage.name_0().rlink();
    auto count        = index_t {0};
    auto result       = &storage.name_0();

    while (current_node != &storage.name_0())
    {
        auto c = compare_module_names (module_name, current_node -> content());
        if (c == less)
        {
            current_node = current_node -> llink ();
        }
        else if (c == greater)
        {
            current_node = current_node -> rlink ();
        }
        else
        {
            result = current_node;
            ++count;
            resume_node = current_node -> rlink ();
            current_node = current_node -> llink ();
        }

        if (current_node == &storage.name_0())
        {
            current_node = resume_node;
            resume_node  = &storage.name_0();
        }
    }

    if (count == 0)
    {
        if (on_no_match) { on_no_match (); }
    }
    else if (count > 1)
    {
            if (on_too_many_matches) { on_too_many_matches (); } 
    }

    return *result;
}

