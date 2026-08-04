#pragma once

#include <string_view>

namespace util
{

template <typename Char_T, typename Derived>
class string_record
{
public:
    using char_type = Char_T;
    using string_view = std::basic_string_view <char_type>;

private:
    Char_T * _start;

    // Safe, as we never hand out the last element to callers
    constexpr auto &
    next () const
    { return *(static_cast<Derived const *>(this) + 1); }

  public:
    explicit string_record (Char_T *start) : _start (start) {}

    auto constexpr length ()  const { return size_t (next ()._start - _start); }
    auto constexpr content () const { return string_view {_start, length ()}; }    
};

}

