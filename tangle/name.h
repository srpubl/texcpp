#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "utility/smallptr.h"
#include "utility/string_record.h"

#include "text.h"

enum ilk_value : uint8_t
{
    normal,     /// ordinary identifiers
    numeric,    /// numeric macros and strings
    simple,     /// simple macros
    parametric  /// parametric macros
};

union equiv_u
{
    text_t * repl_text;
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
    explicit name_t (char_type const *start) : util::string_record <char_type, name_t> (start) {}
    
    auto link ()             const -> name_t * { return _llink; }
    auto llink ()            const -> name_t * { return _llink; }
    auto rlink ()            const -> name_t * { return _rlink; }
    auto chop_link ()        const -> name_t * { return _rlink; }
    auto ilk ()              const             { return _ilk; }
    auto number ()           const             { return _equiv.number - 0x10000; }
    auto replacement_text () const             { return _equiv.repl_text; }

    void set_link             (name_t * value) { this->_llink = value; }
    void set_llink            (name_t * value) { this->_llink = value; }
    void set_rlink            (name_t * value) { this->_rlink = value; }
    void set_chop_link        (name_t * value) { this->_rlink = value; }
    void set_ilk              (ilk_value value){ this->_ilk = value; }
    void set_number           (int32_t value)  { this->_equiv.number = value + 0x10000; }
    void set_replacement_text (text_t &value) { this->_equiv.repl_text = &value; }

    void add_replacement_text (text_t &text) 
    {
        if (replacement_text())
        {
            replacement_text () -> append_continuation (text);
        }
        else
        {
            set_replacement_text (text);
        }
    }

public:
    using optional_reference = std::optional <std::reference_wrapper <name_t>>; 
};

