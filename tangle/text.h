#pragma once

#include "utility/string_record.h"

class text_t : public util::string_record <char32_t, text_t>
{
    text_t * _link = nullptr;

public:
    explicit text_t (char32_t const *start) : util::string_record <char_type, text_t> (start) {}

    auto constexpr link ()  const -> text_t * { return _link; }
    auto constexpr set_link (text_t * value) { this->_link = value; }
};

