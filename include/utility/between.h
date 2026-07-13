#pragma once

#include <concepts>
#include <type_traits>

// Integral types, with branchless optimization
template <std::integral T, std::integral L, std::integral H>
constexpr bool 
is_between (T val, L low, H high) noexcept 
{
    using U = std::make_unsigned_t<T>;
    return static_cast<U>(val - low) <= static_cast<U>(high - low);
}

template <std::totally_ordered T>
constexpr bool 
is_between (const T& val, const T& low, const T& high) 
{
    return val >= low && val <= high;
}
