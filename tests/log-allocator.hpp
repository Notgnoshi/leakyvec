#pragma once
#include <cstdlib>
#include <iostream>
#include <limits>

namespace testing {

template<class T>
struct LogAllocator
{
    using value_type = T;

    // LogAllocator() = default;
    // LogAllocator(const LogAllocator&) = default;

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            throw std::bad_array_new_length();
        }

        if (T* p = reinterpret_cast<T*>(std::malloc(n * sizeof(T))))
        {
            report(p, n);
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        report(p, n, 0);
        std::free(p);
    }

  private:
    void report(T* p, std::size_t n, bool alloc = true) const
    {
        std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n << " bytes at " << std::hex
                  << std::showbase << reinterpret_cast<void*>(p) << std::dec << '\n';
    }
};

template<class T, class U>
bool operator==(const LogAllocator<T>&, const LogAllocator<U>&)
{
    return true;
}

template<class T, class U>
bool operator!=(const LogAllocator<T>&, const LogAllocator<U>&)
{
    return false;
}
}  // namespace testing
