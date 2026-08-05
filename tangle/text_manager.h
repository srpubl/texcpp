#include "utility/string_storage.h"

#include "text.h"

template <>
inline constexpr char const * util::descriptive_type_name<text_t> = "text"; 

template <>
inline constexpr char const * util::descriptive_type_name<text_t::char_type> = "token"; 


class text_manager
{
public:
    using storage_type = util::string_storage <text_t>;
    using string_view = storage_type::string_view; 

public:
    storage_type storage;

public:
    void
    initialize (size_t max_tokens, size_t max_texts);
};
