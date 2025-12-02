find_package(GTest REQUIRED)

add_executable(leakyvec-tests test-allocators.cpp test-leaky-vec.cpp test-vec-wrapper.cpp)
target_compile_features(leakyvec-tests INTERFACE cxx_std_17)
target_link_libraries(leakyvec-tests PUBLIC leakyvec)
target_link_libraries(leakyvec-tests PRIVATE GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(leakyvec-tests)
