#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "range.h"

namespace pascal
{

template <typename Range, typename T>
class array
{
  private:
    static constexpr size_t  TotalSize = static_cast<size_t> (Range::maximum - Range::minimum + 1);

    std::array<T, TotalSize> storage;

    // Helper to validate and map custom Range indices to 0-based layout
    constexpr size_t
    map_index (Range index) const
    {
        return static_cast<size_t> (
            static_cast<int64_t> (index) - static_cast<int64_t> (Range::minimum));
    }

  public:
    using range_type             = Range;
    using value_type             = T;
    using size_type              = size_t;
    using difference_type        = ptrdiff_t;
    using reference              = T &;
    using const_reference        = const T &;
    using pointer                = T *;
    using const_pointer          = const T *;

    using iterator               = typename std::array<T, TotalSize>::iterator;
    using const_iterator         = typename std::array<T, TotalSize>::const_iterator;
    using reverse_iterator       = typename std::array<T, TotalSize>::reverse_iterator;
    using const_reverse_iterator = typename std::array<T, TotalSize>::const_reverse_iterator;

    constexpr reference
    operator[] (Range index) noexcept
    { return storage [map_index (index)]; }

    constexpr const_reference
    operator[] (Range index) const noexcept
    { return storage [map_index (index)]; }

    constexpr reference
    at (Range index)
    { return storage.at (map_index (index)); }

    constexpr const_reference
    at (Range index) const
    { return storage.at (map_index (index)); }

    constexpr reference
    front () noexcept
    { return storage.front (); }

    constexpr const_reference
    front () const noexcept
    { return storage.front (); }

    constexpr reference
    back () noexcept
    { return storage.back (); }

    constexpr const_reference
    back () const noexcept
    { return storage.back (); }

    constexpr pointer
    data () noexcept
    { return storage.data (); }

    constexpr const_pointer
    data () const noexcept
    { return storage.data (); }

    constexpr bool
    empty () const noexcept
    { return storage.empty (); }

    constexpr size_type
    size () const noexcept
    { return TotalSize; }

    constexpr size_type
    max_size () const noexcept
    { return TotalSize; }

    constexpr iterator
    begin () noexcept
    { return storage.begin (); }

    constexpr const_iterator
    begin () const noexcept
    { return storage.begin (); }

    constexpr iterator
    end () noexcept
    { return storage.end (); }

    constexpr const_iterator
    end () const noexcept
    { return storage.end (); }

    constexpr const_iterator
    cbegin () const noexcept
    { return storage.cbegin (); }

    constexpr const_iterator
    cend () const noexcept
    { return storage.cend (); }

    constexpr reverse_iterator
    rbegin () noexcept
    { return storage.rbegin (); }

    constexpr const_reverse_iterator
    rbegin () const noexcept
    { return storage.rbegin (); }

    constexpr reverse_iterator
    rend () noexcept
    { return storage.rend (); }

    constexpr const_reverse_iterator
    rend () const noexcept
    { return storage.rend (); }

    constexpr const_reverse_iterator
    crbegin () const noexcept
    { return storage.crbegin (); }

    constexpr const_reverse_iterator
    crend () const noexcept
    { return storage.crend (); }

    constexpr void
    fill (const T &value)
    { storage.fill (value); }

    constexpr void
    swap (array &other) noexcept (noexcept (storage.swap (other.storage)))
    { storage.swap (other.storage); }
};

template <typename Range, typename T>
constexpr void
swap (array<Range, T> &lhs, array<Range, T> &rhs) noexcept (noexcept (lhs.swap (rhs)))
{ lhs.swap (rhs); }

template <typename Range, typename T>
constexpr bool
operator== (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return std::equal (lhs.begin (), lhs.end (), rhs.begin ()); }

template <typename Range, typename T>
constexpr bool
operator!= (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return !(lhs == rhs); }

template <typename Range, typename T>
constexpr bool
operator< (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return std::lexicographical_compare (lhs.begin (), lhs.end (), rhs.begin (), rhs.end ()); }

template <typename Range, typename T>
constexpr bool
operator<= (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return !(rhs < lhs); }

template <typename Range, typename T>
constexpr bool
operator> (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return rhs < lhs; }

template <typename Range, typename T>
constexpr bool
operator>= (const array<Range, T> &lhs, const array<Range, T> &rhs)
{ return !(lhs < rhs); }

template <int64_t MinIndex, int64_t MaxIndex, typename T>
using int_range_array = array<int_range<MinIndex, MaxIndex>, T>;

}  // namespace pascal
