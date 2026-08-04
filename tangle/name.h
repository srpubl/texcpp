#pragma once

#include <cstdint>

#include "config.h"

// TODO: Remove all of this once text_pointers are done properly 
#include "pascal/range.h"

#include "utility/smallptr.h"
#include "utility/string_record.h"

using text_pointer_t  = pascal::int_range<0, config::max_texts>;

enum ilk_value
{
    normal,     /// ordinary identifiers
    numeric,    /// numeric macros and strings
    simple,     /// simple macros
    parametric  /// parametric macros
};

union equiv_u
{
    text_pointer_t repl_text;
    int32_t number;
};

class name_t : public util::string_record <char8_t, name_t>
{
    using name_p = util::smallptr <name_t>;

    name_p _llink     = nullptr;
    name_p _rlink     = nullptr;

    equiv_u _equiv = {};
    ilk_value _ilk   = normal;

public:
    explicit name_t (auto *start) : util::string_record <char_type, name_t> (start) {}
    
    auto constexpr link ()             const -> name_t * { return _llink; }
    auto constexpr llink ()            const -> name_t * { return _llink; }
    auto constexpr rlink ()            const -> name_t * { return _rlink; }
    auto constexpr chop_link ()        const -> name_t * { return _rlink; }
    auto constexpr ilk ()              const             { return _ilk; }
    auto constexpr number ()           const             { return _equiv.number - 0100000; }
    auto constexpr replacement_text () const             { return _equiv.repl_text; }

    auto constexpr set_link             (name_t * value) { this->_llink = value; }
    auto constexpr set_llink            (name_t * value) { this->_llink = value; }
    auto constexpr set_rlink            (name_t * value) { this->_rlink = value; }
    auto constexpr set_chop_link        (name_t * value) { this->_rlink = value; }
    auto constexpr set_ilk              (auto value)     { this->_ilk = value; }
    auto constexpr set_number           (auto value)     { this->_equiv.number = value + 0100000; }
    auto constexpr set_replacement_text (auto value)     { this->_equiv.repl_text = value; }
};

