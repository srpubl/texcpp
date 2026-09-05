#pragma once

#include <string_view>

namespace util
{

// Instances of this class must be used within the same array and that array must have a sentinel at
// the end such that the last string_record knows where it ends.
template <typename Char_T, typename Derived>
class string_record
{
public:
    using char_type = Char_T;
    using string_view = std::basic_string_view <char_type>;

private:
    Char_T const *const _start;

    // Safe, as we never hand out the last element to callers
    constexpr auto const &
    next () const
    {
        static_assert (std::is_base_of_v<string_record, Derived>, 
                       "Template argument 'Derived' must inherit from string_record");

        return *(static_cast<Derived const *>(this) + 1); 
    }

  public:
    explicit string_record (Char_T const *start) : _start (start) {}

    auto constexpr length ()  const { return size_t (next ()._start - _start); }
    auto constexpr content () const { return string_view {_start, length ()}; }
};

}

