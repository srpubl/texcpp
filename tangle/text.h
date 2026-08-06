#pragma once

#include "utility/smallptr.h"
#include "utility/string_record.h"

class text_t : public util::string_record <char32_t, text_t>
{
    util::smallptr <text_t> _continuation = nullptr;

public:
    explicit text_t (char32_t const *start) : util::string_record <char_type, text_t> (start) {}

    auto constexpr continuation ()  const -> text_t * { return _continuation; }
    auto constexpr set_continuation (text_t * value) { this->_continuation = value; }

    auto constexpr
    append_continuation (text_t & value)
    {
        auto t = this;
        while (t -> continuation()) { t = t -> continuation(); }
        t -> set_continuation (&value);
    }
};

