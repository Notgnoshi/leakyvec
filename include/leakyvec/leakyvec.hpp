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
