# leakyvec

A header-only unsafe C++17 wrapper around a `std::vector` that allows leaking its memory.

## Why?

I came across the need to transfer memory ownership of a C++ `std::vector` to a Rust `Vec` in
another project. But `std::vector` doesn't provide a way to leak its memory, and allocators are
"fun", especially when crossing language boundaries.

This is mostly just an experiment to see if it's possible; I'm not sure I'd recommend using this.

## Usage example

```cpp
#include <leakyvec/leakyvec.hpp>

// TODO
```

## Demo transferring memory ownership from C++ to Rust

* [ ] **TODO:** Demo transferring memory ownership from a C++ `std::vector` allocated with the
      default allocator into a Rust `Vec` using the default global allocator.
* [ ] **TODO:** Demo transferring memory ownership using non-default allocators (requires nightly
      Rust to work with unstable allocator APIs).

## How to build and install?

```sh
cmake -B build
DESTDIR=$PWD/build/install cmake --build build --target install
```

## How to build and run the tests?

Install the [googletest](https://github.com/google/googletest) dependency:

```sh
sudo dnf install gmock-devel    # Fedora
sudo apt install libgmock-dev   # Ubuntu
```

Build and run the tests:

```sh
cmake -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build          # Use CTest
./build/tests/leakyvec-tests    # Run the test binary directly
```
