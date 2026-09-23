#pragma once

#include <new>
#include <vector>

namespace util
{

template <typename T>
struct one_time_allocator 
{
    using value_type = T;
    
    std::size_t size = 0;

    one_time_allocator() = default;
    
    template <typename U> 
    one_time_allocator (const one_time_allocator<U> &) noexcept {}

    [[nodiscard]] T * 
    allocate (std::size_t n) 
    {
        if (size)
            throw std::bad_alloc ();

        return std::allocator<T>{}.allocate(size = n);
    }

    void
    deallocate (T *p, std::size_t n) noexcept 
    {
        std::allocator<T>{}.deallocate(p, n);
    }
};

template <typename T>
using static_vector = std::vector<T, one_time_allocator <T>>;

}

