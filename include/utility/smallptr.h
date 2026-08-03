#pragma once

#include <cstdint>
#include <type_traits>

namespace util
{

template<typename T>
class smallptr 
{
    int32_t offset = 0;

public:
    constexpr smallptr () : offset (0) {}
    constexpr smallptr (std::nullptr_t) : offset (0) {}
    constexpr smallptr (T *target) { assign(target); }
    constexpr smallptr (const smallptr &source) { assign (&*source); }
    constexpr smallptr (smallptr &&source) noexcept 
    { 
        assign (&*source); 
        source.offset = 0;
    }
    
    constexpr smallptr & 
    operator= (T *target) 
    {
        assign (target);
        return *this;
    }

    constexpr smallptr &
    operator= (std::nullptr_t) 
    {
        offset = 0;
        return *this;
    }

    constexpr smallptr & 
    operator= (const smallptr &source)
    {
        assign (&*source);
        return *this;
    }

    constexpr smallptr & operator= (smallptr &&source) noexcept 
    {
        if (this != &source) 
        {
            assign (&*source);
            source.offset = 0; // Safe cleanup of the old moved-from pointer
        }
        return *this;
    }

    constexpr explicit operator bool () const { return offset != 0; }

    T * 
    operator-> () const 
    {
        if (!offset) 
            return nullptr;

        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(this) + offset);
    }

    T& operator*() const { return *(this->operator->()); }
    operator T*() const { return this->operator->(); }

private:
    constexpr void assign(T *target) 
    {
        if (!target) 
        {
            offset = 0;
        } 
        else 
        {
            // Constexpr-safe distance calculation if evaluated at compile-time, 
            // otherwise falls back to bitwise uintptr_t math to avoid Undefined Behavior.
            offset = std::is_constant_evaluated()
                ? static_cast <int32_t> (target - reinterpret_cast <T *> (this))
                : static_cast <int32_t> (reinterpret_cast <uintptr_t> (target) 
                    - reinterpret_cast <uintptr_t> (this));
        }
    }
};

}

