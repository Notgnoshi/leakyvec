#pragma once
#include <tuple>
#include <vector>

namespace leaky {

namespace detail {
    template<typename T, typename Alloc = typename std::vector<T>::allocator_type>
    struct VecWrapper
    {
        using pointer = typename std::vector<T, Alloc>::pointer;
        std::vector<T, Alloc> inner;

        [[nodiscard]] pointer get_data_start() noexcept { return inner.data(); }

        [[nodiscard]] pointer get_data_end() noexcept
        {
            // One past the end of the initialized data
            return inner.data() + inner.size();
        }

        [[nodiscard]] pointer get_capacity_end() noexcept
        {
            // One past the end of the allocated capacity
            return inner.data() + inner.capacity();
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

        /// @brief Set the data start pointer to a new value
        ///
        /// @warning This very likely invalidates the data_end and capacity_end pointers!
        void unsafe_set_data_start(pointer new_start) noexcept
        {
            static_assert(sizeof(inner) >= 3 * sizeof(pointer));
            // inner._M_impl._M_start = new_start;
        }

        /// @brief Set the size of the vector to a new value
        ///
        /// @warning This may invalidate the capacity_end pointer!
        /// @note This requires the data_start pointer be valid.
        void unsafe_set_size(size_t new_size) noexcept
        {
            // std::vector is not guaranteed to be 3 words if it has a non-default allocator.
            // Further, the _M_start pointer isn't the first member in such a case. Unfortunately,
            // the allocator stuff at the beginning of _Vector_impl isn't the same size as the Alloc
            // template parameter, so we can't just assert that
            //
            //     static_assert(sizeof(inner) == sizeof(Alloc) + 3 * sizeof(pointer));
            static_assert(sizeof(inner) >= 3 * sizeof(pointer));

            // NOTE: In libstdc++, std::vector is defined like
            //
            //     struct vector : _Vector_base {};
            //
            //     struct _Vector_base {
            //         _Vector_impl _M_impl;
            //     };
            //     struct _Vector_impl : _Tp_alloc_type, _Vector_impl_data {};
            //     struct _Tp_alloc_type { ... }; // allocator, if present
            //     struct _Vector_impl_data {
            //         pointer _M_start;
            //         pointer _M_finish;
            //         pointer _M_end_of_storage;
            //     };
            //
            // TODO: Look at MSVC and libc++ implementations as well. Specialize the implementation
            // if necessary, otherwise at a minimum add enough static_asserts or unit tests to
            // ensure the layout is as expected.

            // inner._M_impl._M_finish = inner._M_impl._M_start + new_size;
        }

        /// @brief Set the capacity of the vector to a new value
        ///
        /// @warning This may allow writing beyond the allocated memory if used incorrectly!
        /// @note This requires the data_start pointer be valid.
        void unsafe_set_capacity(size_t new_capacity) noexcept
        {
            static_assert(sizeof(inner) >= 3 * sizeof(pointer));
            // inner._M_impl._M_end_of_storage = inner._M_impl._M_start + new_capacity;
        }
    };
}  // namespace detail

/// @brief a wrapper around std::vector that allows leaking its contents and transfering ownership
///
/// @tparam T the element type
/// @tparam Alloc the allocator type
template<typename T, typename Alloc = typename std::vector<T>::allocator_type>
class Vec
{
  private:
    detail::VecWrapper<T, Alloc> m_inner;

  public:
    /// @brief Create a leaky Vec from a std::vector. Takes exclusive ownership.
    Vec(std::vector<T, Alloc>&& vec) noexcept : m_inner(std::move(vec)) {}
    /// @brief Take exclusive ownership from another leaky Vec.
    Vec(Vec&& other) noexcept : m_inner(std::move(other.m_inner)) {}
    /// @brief A Vec owns the internal vector exclusively; no copying.
    Vec(const Vec&) = delete;
    ~Vec() noexcept = default;

    Vec& operator=(const Vec&) = delete;
    Vec& operator=(Vec&& other) noexcept
    {
        m_inner = std::move(other.m_inner);
        return *this;
    }

    /// @brief Take ownership of the std::vector back
    std::vector<T, Alloc> take() noexcept { return std::move(m_inner); }
    /// @brief Get a const reference to the internal std::vector
    const std::vector<T, Alloc>& as_ref() const noexcept { return m_inner; }
    /// @brief Get a mutable reference to the internal std::vector
    std::vector<T, Alloc>& as_mut() noexcept { return m_inner; }

    /// @brief Get the allocator that owns the vector's memory
    constexpr Alloc get_allocator() const noexcept { return m_inner.get_allocator(); }
};  // class Vec
}  // namespace leaky
