#pragma once

#include <cmath>
#include <stdexcept>
#include <utility>

namespace pascal
{

struct checked_policy
{};

struct performance_policy
{};

using default_policy = checked_policy;

template <typename T, T Min, T Max, typename Policy = default_policy>
class range
{
    static_assert (Min <= Max, "Min cannot be greater than Max");

    T value;

    template <std::integral U>
    static constexpr T
    validate (U val)
    {
        if constexpr (std::is_same_v<Policy, checked_policy>)
        {
            if (!contains (val))
                throw std::out_of_range ("Value violates subrange constraints");
        }

        return static_cast<T> (val);
    }

  public:
    template <std::integral U>
        requires std::integral<T>
    constexpr static bool
    contains (U val) noexcept
    { return std::cmp_greater_equal (val, Min) && std::cmp_less_equal (val, Max); }

    template <typename U>
        requires (!std::integral<T>)
    constexpr static bool
    contains (const U &val) noexcept
    { return val >= Min && val <= Max; }

    constexpr static T minimum = Min;
    constexpr static T maximum = Max;

    constexpr static range
    first ()
    { return range (Min); }

    constexpr static range
    last ()
    { return range (Max); }

    using base_type = T;

    constexpr range () : value (Min) {}

    template <std::integral U>
    constexpr explicit range (U val) : value (validate (val))
    {}

    // When the incoming range is guaranteed to fit
    template <typename OtherT, OtherT OtherMin, OtherT OtherMax>
        requires (OtherMin >= Min && OtherMax <= Max)
    constexpr range (const range<OtherT, OtherMin, OtherMax> &other) noexcept
        : value (static_cast<T> (other))
    {}

    // If the incoming range can exceed bounds
    template <typename OtherT, OtherT OtherMin, OtherT OtherMax>
        requires (OtherMin<Min || OtherMax> Max)
    constexpr range (const range<OtherT, OtherMin, OtherMax> &other)
        : value (validate (static_cast<OtherT> (other)))
    {}

    constexpr
    operator T () const
    { return value; }

    template <typename T2, T2 Min2, T2 Max2, typename Policy2>
    constexpr range &
    operator+= (const range<T2, Min2, Max2, Policy2> &rhs) noexcept
    {
        value = validate (value + static_cast<T2> (rhs));
        return *this;
    }

    template <typename T2, T2 Min2, T2 Max2, typename Policy2>
    constexpr range &
    operator-= (const range<T2, Min2, Max2, Policy2> &rhs) noexcept
    {
        value = validate (value - static_cast<T2> (rhs));
        return *this;
    }

    constexpr range &
    operator++ ()
    {
        value = validate (value + 1);
        return *this;
    }

    constexpr range
    operator++ (int)
    {
        auto result = *this;
        value       = validate (value + 1);
        return result;
    }

    constexpr range &
    operator-- ()
    {
        value = validate (value - 1);
        return *this;
    }

    constexpr range
    operator-- (int)
    {
        auto result = *this;
        value       = validate (value - 1);
        return result;
    }

    template <typename OtherT, OtherT OtherMin, OtherT OtherMax, typename OtherPolicy>
        requires std::integral<T> && std::integral<OtherT>
    constexpr bool
    operator== (const range<OtherT, OtherMin, OtherMax, OtherPolicy> &rhs) const noexcept
    { return std::cmp_equal (value, static_cast<OtherT> (rhs)); }

    template <typename OtherT, OtherT OtherMin, OtherT OtherMax, typename OtherPolicy>
        requires std::integral<T> && std::integral<OtherT>
    constexpr auto
    operator<=> (const range<OtherT, OtherMin, OtherMax, OtherPolicy> &rhs) const noexcept
    {
        auto lhs_val = value;
        auto rhs_val = static_cast<OtherT> (rhs);

        if (std::cmp_less (lhs_val, rhs_val))
            return std::strong_ordering::less;

        if (std::cmp_greater (lhs_val, rhs_val))
            return std::strong_ordering::greater;

        return std::strong_ordering::equal;
    }

    template <typename OtherT, OtherT OtherMin, OtherT OtherMax, typename OtherPolicy>
        requires (!(std::integral<T> && std::integral<OtherT>) )
    constexpr bool
    operator== (const range<OtherT, OtherMin, OtherMax, OtherPolicy> &rhs) const noexcept
    { return value == static_cast<OtherT> (rhs); }

    template <typename OtherT, OtherT OtherMin, OtherT OtherMax, typename OtherPolicy>
        requires (!(std::integral<T> && std::integral<OtherT>) )
    constexpr auto
    operator<=> (const range<OtherT, OtherMin, OtherMax, OtherPolicy> &rhs) const noexcept
    { return value <=> static_cast<OtherT> (rhs); }

    template <typename U>
        requires std::integral<T> && std::integral<U>
    constexpr bool
    operator== (const U &rhs) const noexcept
    { return std::cmp_equal (value, rhs); }

    template <typename U>
        requires std::integral<T> && std::integral<U>
    constexpr auto
    operator<=> (const U &rhs) const noexcept
    {
        if (std::cmp_less (value, rhs))
            return std::strong_ordering::less;

        if (std::cmp_greater (value, rhs))
            return std::strong_ordering::greater;

        return std::strong_ordering::equal;
    }

    template <typename U>
        requires (!(std::integral<T> && std::integral<U>) )
    constexpr bool
    operator== (const U &rhs) const noexcept
    { return value == rhs; }

    template <typename U>
        requires (!(std::integral<T> && std::integral<U>) )
    constexpr auto
    operator<=> (const U &rhs) const noexcept
    { return value <=> rhs; }
};

namespace internal
{
    template <int64_t Min, int64_t Max>
    struct required_integer_type
    {
      private:
        static constexpr bool is_signed = (Min < 0);

        using signed_type               = typename std::conditional<
            (Min >= -128 && Max <= 127),
            int8_t,
            typename std::conditional<
                (Min >= -32768 && Max <= 32767),
                int16_t,
                typename std::
                    conditional<(Min >= -2147483648LL && Max <= 2147483647LL), int32_t, int64_t>::type>::
                type>::type;

        using unsigned_type = typename std::conditional<
            (Max <= 255),
            uint8_t,
            typename std::conditional<
                (Max <= 65535),
                uint16_t,
                typename std::conditional<(Max <= 4294967295ULL), uint32_t, uint64_t>::type>::type>::
            type;

      public:
        using type = typename std::conditional<is_signed, signed_type, unsigned_type>::type;
    };
}  // namespace internal

template <int64_t Min, int64_t Max>
using int_range = range<typename internal::required_integer_type<Min, Max>::type, Min, Max>;

namespace internal
{
    template <char... Chars>
    constexpr int64_t
    parse_numeric_literal ()
    {
        int64_t val = 0;
        ((val = val * 10 + (Chars - '0')), ...);
        return val;
    }
}  // namespace internal

template <char... Chars>
constexpr auto
operator""_r () noexcept
{
    constexpr int64_t i = internal::parse_numeric_literal<Chars...> ();
    return int_range<i, i> {i};
}

template <typename T, T Min, T Max, typename Policy = default_policy>
constexpr auto
make_range (T value) noexcept
{
    using ResultType = typename internal::required_integer_type<Min, Max>::type;

    return range<ResultType, static_cast<ResultType> (Min), static_cast<ResultType> (Max), Policy> {
        static_cast<ResultType> (value)
    };
}

template <typename T, T Min, T Max, typename Policy>
constexpr auto
operator- (const range<T, Min, Max, Policy> &rhs) noexcept
{
    constexpr int64_t NewMin = -static_cast<int64_t> (Max);
    constexpr int64_t NewMax = -static_cast<int64_t> (Min);

    return make_range<int64_t, NewMin, NewMax, Policy> (-static_cast<T> (rhs));
}

template <
    typename T1,
    T1 Min1,
    T1 Max1,
    typename Policy1,
    typename T2,
    T2 Min2,
    T2 Max2,
    typename Policy2>
constexpr auto
operator+ (const range<T1, Min1, Max1, Policy1> &lhs, const range<T2, Min2, Max2, Policy2> &rhs) noexcept
{
    constexpr int64_t NewMin = static_cast<int64_t> (Min1) + static_cast<int64_t> (Min2);
    constexpr int64_t NewMax = static_cast<int64_t> (Max1) + static_cast<int64_t> (Max2);

    return make_range<int64_t, NewMin, NewMax, Policy1> (static_cast<T1> (lhs) + static_cast<T2> (rhs));
}

template <
    typename T1,
    T1 Min1,
    T1 Max1,
    typename Policy1,
    typename T2,
    T2 Min2,
    T2 Max2,
    typename Policy2>
constexpr auto
operator- (const range<T1, Min1, Max1, Policy1> &lhs, const range<T2, Min2, Max2, Policy2> &rhs) noexcept
{
    constexpr int64_t NewMin = static_cast<int64_t> (Min1) - static_cast<int64_t> (Max2);
    constexpr int64_t NewMax = static_cast<int64_t> (Max1) - static_cast<int64_t> (Min2);

    return make_range<int64_t, NewMin, NewMax, Policy1> (static_cast<T1> (lhs) - static_cast<T2> (rhs));
}

template <
    typename T1,
    T1 Min1,
    T1 Max1,
    typename Policy1,
    typename T2,
    T2 Min2,
    T2 Max2,
    typename Policy2>
constexpr auto
operator% (const range<T1, Min1, Max1, Policy1> &lhs, const range<T2, Min2, Max2, Policy2> &rhs) noexcept
{
    // Because std::abs is not constexpr
    constexpr auto abs = [] (auto x) { return x < 0 ? -x : x; };

    static_assert (
        Min2 != 0LL || Max2 != 0LL,
        "Division or modulo by a guaranteed zero range is illegal.");

    // In C++ the sign of the result of % depends only on lhs, 
    // i.e., we can assume the range of rhs to be positive.
    constexpr auto max2   = std::max (abs (Min2), abs (Max2)) - 1;

    constexpr auto NewMin = Min1 >= 0LL ? 0LL : Min1 > -max2 ? Min1 : -max2;
    constexpr auto NewMax = Max2 <= 0LL ? 0LL : Max1 < max2 ? Max1 : max2;

    return make_range<int64_t, NewMin, NewMax, Policy1> (static_cast<T1> (lhs) % static_cast<T2> (rhs));
}

}  // namespace pascal
