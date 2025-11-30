#pragma
#include "log-allocator.hpp"

#include <memory>

#include <gmock/gmock.h>

namespace testing {

template<typename T>
struct MockAllocatorNonCopyable
{
    using value_type = T;

    MockAllocatorNonCopyable()
    {
        // Defer allocation implementation to the LogAllocator
        ON_CALL(*this, allocate).WillByDefault([this](std::size_t n) {
            return m_log_alloc.allocate(n);
        });
        ON_CALL(*this, deallocate).WillByDefault([this](T* that, std::size_t n) {
            return m_log_alloc.deallocate(that, n);
        });
    }

    MOCK_METHOD(T*, allocate, (std::size_t));
    MOCK_METHOD(void, deallocate, (T*, std::size_t));

  private:
    LogAllocator<T> m_log_alloc;
};

template<typename T>
struct MockAllocator
{
    // passing the allocator to std::vector's constructor requires it to be copyable. So we stuff
    // the Mock into a shared pointer so that we can copy it around. But the pointer has to be
    // public, so we can pass it into EXPECT_CALL as needed.
    std::shared_ptr<StrictMock<MockAllocatorNonCopyable<T>>> mock;

    using value_type = T;
    MockAllocator() : mock(std::make_shared<StrictMock<MockAllocatorNonCopyable<T>>>()) {}

    T* allocate(std::size_t n) { return mock->allocate(n); }
    void deallocate(T* p, std::size_t n) { mock->deallocate(p, n); }
};

template<class T, class U>
bool operator==(const MockAllocator<T>&, const MockAllocator<U>&)
{
    return true;
}

template<class T, class U>
bool operator!=(const MockAllocator<T>&, const MockAllocator<U>&)
{
    return false;
}

}  // namespace testing
