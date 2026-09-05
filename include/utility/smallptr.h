#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace util
{


/// Can only be used for pointers within the same array (or vector if this is guaranteed to never be
/// resized).
/// An instance cannot express pointing to itself, this would be equivalent to a null pointer. The
/// implementation does not check this case, so beware.    
template<typename T>
class smallptr 
{
    int32_t offset = 0;

public:
    smallptr () : offset (0) {}
    smallptr (std::nullptr_t) : offset (0) {}
    smallptr (T *target) { assign (target); }
    smallptr (const smallptr &source) { assign (source.get()); }
    smallptr (smallptr &&source) noexcept 
    { 
        assign (source.get()); 
        source.offset = 0;
    }
    
    smallptr & 
    operator= (T *target) 
    {
        assign (target);
        return *this;
    }

    smallptr &
    operator= (std::nullptr_t) 
    {
        offset = 0;
        return *this;
    }

    smallptr & 
    operator= (const smallptr &source)
    {
        assign (source.get());
        return *this;
    }

    smallptr & 
    operator= (smallptr &&source) noexcept 
    {
        if (this != &source) 
        {
            assign (source.get());
            source.offset = 0;
        }
        return *this;
    }

    explicit 
    operator bool () const { return offset != 0; }

    T * 
    operator-> () const { return get (); }
    T & 
    operator* () const { return *get (); }
    operator T* () const { return get (); }

    T *
    get () const
    {
        if (!offset) 
            return nullptr;

        return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(this) + offset);        
    }

private:
    void assign (const T *target) 
    {
        if (!target) 
        {
            offset = 0;
        } 
        else 
        {
            offset = 
                static_cast <int32_t> (
                      reinterpret_cast <uintptr_t> (target) 
                    - reinterpret_cast <uintptr_t> (this));
        }
    }
};

static_assert(std::is_nothrow_move_constructible_v<smallptr<int>>, 
              "util::smallptr must be noexcept move constructible for container optimization");

static_assert(std::is_nothrow_move_assignable_v<smallptr<int>>, 
              "util::smallptr must be noexcept move assignable for container optimization");
}

