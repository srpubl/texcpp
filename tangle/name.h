#pragma once

#include <cstdint>

// TODO: Remove all of this once text_pointers are done properly 
#include "pascal/range.h"
constexpr size_t max_texts    = 2000;   /// number of replacement texts, must be < 10240
using text_pointer_t  = pascal::int_range<0, max_texts>;
using index_t         = uint32_t;  /// used to store indices in arrays

class name_t;

enum ilk_value
{
    normal,     /// ordinary identifiers
    numeric,    /// numeric macros and strings
    simple,     /// simple macros
    parametric  /// parametric macros
};

union equiv_u
{
    name_t *chop_link;
    text_pointer_t repl_text;
    int32_t number;
};

class name_t
{
    char8_t * _start;
    name_t *_link = nullptr;
    name_t * _llink;
    name_t * _rlink;

    equiv_u _equiv = {};
    ilk_value _ilk   = normal;

    // Safe, as we never hand out the last element to callers
    constexpr auto &
    next () const
    { return *(this + 1); }

  public:
    name_t (char8_t *start, name_t * name_0) 
    : _start (start), _llink(name_0), _rlink(name_0) 
    {}
    
    auto constexpr length () const { return size_t (next ()._start - _start); }
    auto constexpr content () const -> std::u8string_view { return {_start, length ()}; }
    auto constexpr link () const { return _link; }
    auto constexpr set_link (auto value) { this->_link = value; }
    auto constexpr llink () const { return _llink; }
    auto constexpr set_llink (auto value) { this->_llink = value; }
    auto constexpr rlink () const { return _rlink; }
    auto constexpr set_rlink (auto value) { this->_rlink = value; }
    auto constexpr ilk () const { return _ilk; }
    auto constexpr set_ilk (auto value) { this->_ilk = value; }
    auto constexpr chop_link () const { return _equiv.chop_link; }
    auto constexpr set_chop_link (auto value) { this->_equiv.chop_link = value; }
    auto constexpr number () const { return _equiv.number - 0100000; }
    auto constexpr set_number (auto value) { this->_equiv.number = value + 0100000; }
    auto constexpr replacement_text () const { return _equiv.repl_text; }
    auto constexpr set_replacement_text (auto value) { this->_equiv.repl_text = value; }
};

