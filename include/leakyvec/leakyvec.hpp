#pragma once
#include <tuple>
#include <type_traits>
#include <vector>

#ifdef NDEBUG
    #define LEAKY_DEBUG_ASSERT(x)                                                                  \
        do                                                                                         \
        {                                                                                          \
            static_cast<void>(x);                                                                  \
        } while (0)
#else
    #include <cassert>
    #define LEAKY_DEBUG_ASSERT(x) assert(x)
#endif

namespace leaky {

namespace detail {
    template<typename T, typename Alloc = typename std::vector<T>::allocator_type>
    struct VecWrapper
    {
        using pointer = typename std::vector<T, Alloc>::pointer;
        std::vector<T, Alloc> inner;

        // NOTE: In libstdc++, std::vector is defined like
        //
        //     struct vector : _Vector_base {};
        //
        //     struct _Vector_base {
        //         _Vector_impl _M_impl;
        //     };
        //     struct _Vector_impl : _Tp_alloc_type, _Vector_impl_data {};
        //     struct _Tp_alloc_type { ... }; // allocator, if present, zero-sized otherwise
        //     struct _Vector_impl_data {
        //         pointer _M_start;
        //         pointer _M_finish;
        //         pointer _M_end_of_storage;
        //     };
        //
        // TODO: Look at MSVC and libc++ implementations as well!
        template<typename A = Alloc, typename D = std::allocator<T>>
        [[nodiscard]] constexpr std::enable_if_t<std::is_same_v<A, D>, size_t>
        get_data_ptr_offset() const noexcept
        {
            // The default allocator is not stored in the std::vector
            static_assert(sizeof(inner) == 3 * sizeof(pointer));
            return 0;
        }

        template<typename A = Alloc, typename D = std::allocator<T>>
        [[nodiscard]] constexpr std::enable_if_t<!std::is_same_v<A, D>, size_t>
        get_data_ptr_offset() const noexcept
        {
            // Any non-default allocators are stored at the beginning of the std::vector
            static_assert(sizeof(inner) == sizeof(Alloc) + 3 * sizeof(pointer));
            // Return the offset in words, not in bytes, because when we do pointer arithmetic, it
            // increments by sizeof(pointer), not in bytes.
            return sizeof(Alloc) / sizeof(pointer);
        }

        [[nodiscard]] pointer get_data_start() noexcept { return inner.data(); }
        [[nodiscard]] pointer* get_data_start_ptr() noexcept
        {
            // The offset is implementation defined, and depends on whether the std::vector uses the
            // default allocator.
            auto* ptr = reinterpret_cast<pointer*>(&inner) + get_data_ptr_offset() + 0;
            LEAKY_DEBUG_ASSERT(get_data_start() == *ptr);
            return ptr;
        };

        /// @brief Set the data start pointer to a new value
        ///
        /// @warning This very likely invalidates the data_end and capacity_end pointers!
        void unsafe_set_data_start(pointer new_start) noexcept
        {
            // inner._M_impl._M_start = new_start;
            auto* data_start_ptr = get_data_start_ptr();
            *data_start_ptr = new_start;
            LEAKY_DEBUG_ASSERT(inner.data() == new_start);
        }

        [[nodiscard]] pointer get_data_end() noexcept
        {
            // One past the end of the initialized data
            return get_data_start() + inner.size();
        }
        [[nodiscard]] pointer* get_data_end_ptr() noexcept
        {
            auto* ptr = reinterpret_cast<pointer*>(&inner) + get_data_ptr_offset() + 1;
            LEAKY_DEBUG_ASSERT(get_data_end() == *ptr);
            return ptr;
        }

        /// @brief Set the size of the vector to a new value
        ///
        /// @warning This may invalidate the capacity_end pointer!
        /// @note This requires the data_start pointer be valid.
        void unsafe_set_size(size_t new_size) noexcept
        {
            // inner._M_impl._M_finish = inner._M_impl._M_start + new_size;
            auto* data_end_ptr = get_data_end_ptr();
            *data_end_ptr = get_data_start() + new_size;
            LEAKY_DEBUG_ASSERT(inner.size() == new_size);
        }

        [[nodiscard]] pointer get_capacity_end() noexcept
        {
            // One past the end of the allocated capacity
            return get_data_start() + inner.capacity();
        }
        [[nodiscard]] pointer* get_capacity_end_ptr() noexcept
        {
            auto* ptr = reinterpret_cast<pointer*>(&inner) + get_data_ptr_offset() + 2;
            LEAKY_DEBUG_ASSERT(get_capacity_end() == *ptr);
            return ptr;
        }

        /// @brief Set the capacity of the vector to a new value
        ///
        /// @warning This may allow writing beyond the allocated memory if used incorrectly!
        /// @note This requires the data_start pointer be valid.
        void unsafe_set_capacity(size_t new_capacity) noexcept
        {
            // inner._M_impl._M_end_of_storage = inner._M_impl._M_start + new_capacity;
            auto* capacity_end_ptr = get_capacity_end_ptr();
            *capacity_end_ptr = get_data_start() + new_capacity;
            LEAKY_DEBUG_ASSERT(inner.capacity() == new_capacity);
        }

        static VecWrapper<T, Alloc>
        unsafe_from_parts(std::tuple<pointer, size_t, size_t, Alloc> parts) noexcept
        {
            return unsafe_from_parts(
                std::get<0>(parts), std::get<1>(parts), std::get<2>(parts), std::get<3>(parts));
        }

        static VecWrapper<T, Alloc> unsafe_from_parts(pointer data_start,
                                                      size_t size,
                                                      size_t capacity,
                                                      Alloc alloc = Alloc()) noexcept
        {
            VecWrapper<T, Alloc> wrapper{std::vector<T, Alloc>(alloc)};
            wrapper.unsafe_set_data_start(data_start);
            wrapper.unsafe_set_size(size);
            wrapper.unsafe_set_capacity(capacity);
            return wrapper;
        }

        [[nodiscard]] std::tuple<pointer, size_t, size_t, Alloc> leak_into_parts() noexcept
        {
            pointer data_start = inner.data();
            const auto size = inner.size();
            const auto capacity = inner.capacity();
            const auto retval = std::make_tuple(data_start, size, capacity, inner.get_allocator());

            unsafe_set_data_start(nullptr);
            unsafe_set_size(0);
            unsafe_set_capacity(0);

            return retval;
        }
    };
}  // namespace detail

/// @brief a wrapper around std::vector that allows leaking its contents and transfering ownership
///
/// @tparam T the element type
/// @tparam Alloc the allocator type
///
/// @note This helper class takes exclusive ownership of the given std::vector for the sole purpose
/// of leaking its contents. It is not intended to be a general-purpose vector wrapper.
///
/// Example usage:
///
/// ```cpp
/// auto vec = std::vector<int>{1, 2, 3, 4};
/// auto leaky_vec = leaky::Vec<int>(std::move(vec));
/// auto parts = leaky_vec.leak();
///
/// // ...
///
/// auto leaky_vec2 = leaky::Vec<int>::from_parts(parts);
/// auto vec2 = leaky_vec2.take();
/// ```
template<typename T, typename Alloc = typename std::vector<T>::allocator_type>
class Vec
{
  private:
    // Hide the yucky pointer math and helper methods, but still enable unit testing them
    detail::VecWrapper<T, Alloc> m_inner;

    Vec(detail::VecWrapper<T, Alloc>&& wrapper) noexcept : m_inner{std::move(wrapper)} {}

  public:
    using pointer = typename std::vector<T, Alloc>::pointer;

    /// @brief Create a leaky Vec from a std::vector. Takes exclusive ownership.
    Vec(std::vector<T, Alloc>&& vec) noexcept : m_inner{std::move(vec)} {}
    /// @brief Take exclusive ownership from another leaky Vec.
    Vec(Vec&& other) noexcept : m_inner(std::move(other.m_inner)) {}
    /// @brief A Vec owns the internal vector exclusively; no copying.
    Vec(const Vec&) = delete;
    ~Vec() noexcept = default;

    /// @brief Create a leaky Vec from its raw parts returned by `leak()`
    static Vec from_parts(std::tuple<pointer, size_t, size_t, Alloc> parts) noexcept
    {
        return Vec(std::move(detail::VecWrapper<T, Alloc>::unsafe_from_parts(parts)));
    }

    Vec& operator=(const Vec&) = delete;
    Vec& operator=(Vec&& other) noexcept
    {
        m_inner = std::move(other.m_inner);
        return *this;
    }

    /// @brief Take ownership of the std::vector back
    ///
    /// @note After calling this method, the internal std::vector is left in an empty state.
    std::vector<T, Alloc> take() noexcept { return std::move(m_inner.inner); }
    /// @brief Get a const reference to the internal std::vector
    const std::vector<T, Alloc>& as_ref() const noexcept { return m_inner.inner; }
    /// @brief Get a mutable reference to the internal std::vector
    std::vector<T, Alloc>& as_mut() noexcept { return m_inner.inner; }

    /// @brief Get the allocator that owns the vector's memory
    [[nodiscard]] constexpr Alloc get_allocator() const noexcept { return m_inner.get_allocator(); }

    /// @brief Leak the internal vector as its raw parts
    ///
    /// The tuple contains, in order:
    /// 1. pointer to the start of the vector's data block
    /// 2. size of the vector (number of initialized elements)
    /// 3. capacity of the vector (number of allocated elements)
    /// 4. the allocator that owns the vector's memory block
    ///
    /// @note After calling this method, the internal std::vector is left in an empty state.
    [[nodiscard]] std::tuple<pointer, size_t, size_t, Alloc> leak() noexcept
    {
        return m_inner.leak_into_parts();
    }
};  // class Vec
}  // namespace leaky
