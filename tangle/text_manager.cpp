#include "text_manager.h"

template <>
inline constexpr char const * util::descriptive_type_name<text_t> = "text"; 

template <>
inline constexpr char const * util::descriptive_type_name<text_t::char_type> = "token"; 



