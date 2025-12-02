#include "log-allocator.hpp"
#include "mock-allocator.hpp"

#include <leakyvec/leakyvec.hpp>

#include <tuple>

#include <gmock/gmock.h>

/// Test contiguous data block pointers for a one-byte type
TEST(VecWrapperTests, uint8_t)
{
    auto v = std::vector<uint8_t, testing::LogAllocator<uint8_t>>{1, 2, 3, 4};
    v.reserve(10);

    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);

    leaky::detail::VecWrapper<uint8_t, testing::LogAllocator<uint8_t>> wrapper{std::move(v)};

    const auto* data_start = wrapper.get_data_start();
    ASSERT_EQ(data_start, wrapper.inner.data());

    const auto* one_past_data_end = wrapper.get_data_end();
    const auto* data_end = one_past_data_end - 1;
    ASSERT_EQ(data_end[0], 4);
    ASSERT_EQ(one_past_data_end - data_start, 4);

    const auto* capacity_end = wrapper.get_capacity_end();
    ASSERT_EQ(capacity_end - data_start, 10);
}

/// Test contiguous data block pointers for a word-sized type
TEST(VecWrapperTests, uint64_t)
{
    auto v = std::vector<uint64_t>{1, 2, 3, 4};
    v.reserve(10);

    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 10);

    leaky::detail::VecWrapper<uint64_t> wrapper{std::move(v)};

    const auto* data_start = wrapper.get_data_start();
    ASSERT_EQ(data_start, wrapper.inner.data());

    const auto* one_past_data_end = wrapper.get_data_end();
    const auto* data_end = one_past_data_end - 1;
    ASSERT_EQ(data_end[0], 4);
    ASSERT_EQ(one_past_data_end - data_start, 4);

    const auto* capacity_end = wrapper.get_capacity_end();
    ASSERT_EQ(capacity_end - data_start, 10);
}

/// Verify assumptions about the memory layout of a std::vector with the default allocator
TEST(VecWrapperTests, DefaultAllocMemoryLayout)
{
    auto v = std::vector<int>{1, 2, 3, 4};
    v.reserve(10);

    auto wrapper = leaky::detail::VecWrapper<int>{std::move(v)};
    ASSERT_EQ(wrapper.get_data_ptr_offset(), 0);

    auto* data_start_ptr = wrapper.get_data_start_ptr();
    ASSERT_EQ(*data_start_ptr, wrapper.inner.data());

    auto* data_end_ptr = wrapper.get_data_end_ptr();
    ASSERT_EQ(*data_end_ptr, wrapper.inner.data() + wrapper.inner.size());

    auto* capacity_end_ptr = wrapper.get_capacity_end_ptr();
    ASSERT_EQ(*capacity_end_ptr, wrapper.inner.data() + wrapper.inner.capacity());
}

/// Verify assumptions about the memory layout of a std::vector with a custom allocator
TEST(VecWrapperTests, CustomAllocMemoryLayout)
{
    testing::MockAllocator<int> alloc;
    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                 // initial allocation
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);   // deallocate
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);                // resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);  // drop

    auto v = std::vector<int, testing::MockAllocator<int>>{{1, 2, 3, 4}, alloc};
    v.reserve(10);

    auto wrapper = leaky::detail::VecWrapper<int, testing::MockAllocator<int>>{std::move(v)};
    ASSERT_EQ(wrapper.get_data_ptr_offset(), sizeof(alloc) / sizeof(int*));

    auto* data_start_ptr = wrapper.get_data_start_ptr();
    ASSERT_EQ(*data_start_ptr, wrapper.inner.data());

    auto* data_end_ptr = wrapper.get_data_end_ptr();
    ASSERT_EQ(*data_end_ptr, wrapper.inner.data() + wrapper.inner.size());

    auto* capacity_end_ptr = wrapper.get_capacity_end_ptr();
    ASSERT_EQ(*capacity_end_ptr, wrapper.inner.data() + wrapper.inner.capacity());
}

/// Test that we can set the vector's data pointer, size, and capacity
TEST(VecWrapperTests, DefaultAllocSetters)
{
    auto v = std::vector<int>{1, 2, 3, 4};
    v.reserve(10);

    const auto original_capacity = v.capacity();
    ASSERT_EQ(original_capacity, 10);
    ASSERT_EQ(v.size(), 4);

    auto wrapper = leaky::detail::VecWrapper<int>{std::move(v)};
    auto* original_data_start = wrapper.get_data_start();
    const auto* original_data_end = wrapper.get_data_end();
    const auto* original_capacity_end = wrapper.get_capacity_end();

    wrapper.unsafe_set_size(3);
    ASSERT_EQ(wrapper.inner.size(), 3);

    // Would introduce a memory leak if we left this dangling
    wrapper.unsafe_set_capacity(3);
    ASSERT_EQ(wrapper.inner.capacity(), 3);

    wrapper.unsafe_set_capacity(original_capacity);
    ASSERT_EQ(wrapper.inner.capacity(), original_capacity);
    ASSERT_EQ(wrapper.get_data_end(), original_data_end - 1);
    ASSERT_EQ(wrapper.get_capacity_end(), original_capacity_end);

    // Can't deallocate without an explosion, so we have to put it back
    wrapper.unsafe_set_data_start(original_data_start + 1);
    ASSERT_EQ(wrapper.inner.data(), original_data_start + 1);
    ASSERT_EQ(wrapper.get_data_start(), original_data_start + 1);

    wrapper.unsafe_set_data_start(original_data_start);
}

/// Test that we can set the vector's data pointer, size, and capacity
TEST(VecWrapperTests, CustomAllocSetters)
{
    testing::MockAllocator<int> alloc;
    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                 // initial allocation
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);   // deallocate
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);                // resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);  // drop

    auto v = std::vector<int, testing::MockAllocator<int>>{{1, 2, 3, 4}, alloc};
    v.reserve(10);
    const auto original_capacity = v.capacity();

    auto wrapper = leaky::detail::VecWrapper<int, testing::MockAllocator<int>>{std::move(v)};
    auto* original_data_start = wrapper.get_data_start();
    const auto* original_data_end = wrapper.get_data_end();
    const auto* original_capacity_end = wrapper.get_capacity_end();

    wrapper.unsafe_set_size(3);
    ASSERT_EQ(wrapper.inner.size(), 3);

    // Would introduce a memory leak if we left this dangling
    wrapper.unsafe_set_capacity(3);
    ASSERT_EQ(wrapper.inner.capacity(), 3);

    wrapper.unsafe_set_capacity(original_capacity);
    ASSERT_EQ(wrapper.inner.capacity(), original_capacity);
    ASSERT_EQ(wrapper.get_data_end(), original_data_end - 1);
    ASSERT_EQ(wrapper.get_capacity_end(), original_capacity_end);

    // Can't deallocate without an explosion, so we have to put it back
    wrapper.unsafe_set_data_start(original_data_start + 1);
    ASSERT_EQ(wrapper.inner.data(), original_data_start + 1);
    ASSERT_EQ(wrapper.get_data_start(), original_data_start + 1);

    wrapper.unsafe_set_data_start(original_data_start);
}

/// Test that we can take apart and reconstruct the std::vector from its internals
TEST(VecWrapperTests, ToAndFromParts)
{
    testing::MockAllocator<int> alloc;
    EXPECT_CALL(*alloc.mock, allocate(4)).Times(1);                 // initial allocation
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 4)).Times(1);   // deallocate
    EXPECT_CALL(*alloc.mock, allocate(10)).Times(1);                // resize
    EXPECT_CALL(*alloc.mock, deallocate(testing::_, 10)).Times(1);  // drop

    auto alloc2 = alloc;  // copy
    auto parts = std::make_tuple<int*, size_t, size_t, testing::MockAllocator<int>>(
        nullptr, 0, 0, std::move(alloc2));
    int* original_data_start = nullptr;
    int* original_data_end = nullptr;
    int* original_capacity_end = nullptr;

    {
        auto v = std::vector<int, testing::MockAllocator<int>>{{1, 2, 3, 4}, alloc};
        v.reserve(10);

        auto wrapper = leaky::detail::VecWrapper<int, testing::MockAllocator<int>>{std::move(v)};
        original_data_start = wrapper.get_data_start();
        original_data_end = wrapper.get_data_end();
        original_capacity_end = wrapper.get_capacity_end();

        // Orphan the vector's memory; would leak if we left this dangling
        parts = wrapper.leak_into_parts();
    }  // The vector is dropped here, but we've kept its memory alive!

    // Reconstruct the vector from the original memory parts; should not do any more allocations,
    // and should have the exact same pointers as before;
    auto new_wrapper =
        leaky::detail::VecWrapper<int, testing::MockAllocator<int>>::unsafe_from_parts(parts);

    ASSERT_EQ(new_wrapper.get_data_start(), original_data_start);
    ASSERT_EQ(new_wrapper.get_data_end(), original_data_end);
    ASSERT_EQ(new_wrapper.get_capacity_end(), original_capacity_end);
}
