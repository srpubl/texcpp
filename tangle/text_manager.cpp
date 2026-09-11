#include "text_manager.h"

void
text_manager::initialize (size_t max_tokens, size_t max_texts)
{
    storage.initialize (max_tokens, max_texts);
}

template <>
inline constexpr char const * util::descriptive_type_name<text_t> = "text"; 

template <>
inline constexpr char const * util::descriptive_type_name<text_t::char_type> = "token"; 



