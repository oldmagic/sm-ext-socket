# Copilot Instructions: SourceMod Socket Extension

## Project Overview

**SourceMod Socket Extension v2.0** - A C++ networking extension providing TCP/UDP socket operations for SourceMod game server plugins. Currently undergoing modernization from C++98 to C++17/20 standards with 95% core implementation complete.

- **Language:** C++17 (C++20 optional)
- **Size:** ~2,000 lines of core code (excluding SDK)
- **Key Dependencies:** Boost.Asio 1.70+ (or standalone Asio), SourceMod SDK 1.12+, MetaMod:Source 1.11+
- **Target:** Linux game servers (primarily 32-bit ELF) and Windows
- **Build System:** CMake 3.15+

## Critical Architecture Patterns

### 1. Template-Based Protocol Abstraction
`Socket.h` uses **template specialization** for TCP vs UDP:
```cpp
template <class SocketType>  // SocketType = asio::ip::tcp or asio::ip::udp
class Socket { /* ... */ };
```
When modifying socket operations, always update **both protocol specializations** in `Socket.cpp` (TCP at ~line 100, UDP at ~line 400). Search for `template class Socket<asio::ip::tcp::socket>` to find instantiation points.

### 2. Modern C++ Memory Management (CRITICAL)
**Never use raw pointers, new, or delete.** The codebase is 100% modernized to:
- `std::unique_ptr` for owned resources (sockets, I/O service)
- `std::shared_ptr` NOT used (explicit ownership transfer via move semantics)
- Move semantics enforced via deleted copy constructors
- RAII patterns throughout

Example from `SocketHandler.cpp`:
```cpp
std::unique_ptr<asio::io_service> ioService;  // NOT asio::io_service*
```

### 3. Type-Safe Enums with Backward Compatibility
All enums are `enum class` (e.g., `SM_SocketType`, `SM_ErrorType`) in `Define.h`. For SourcePawn API compatibility, legacy integer constants exist as `constexpr` shims at bottom of `Define.h`. When adding options, update **both** the enum class and the shim layer.

### 4. Thread-Safe Callback Queue
`CallbackHandler.cpp` uses `std::mutex` + `std::deque` for callbacks. When touching callback code:
- Always lock `mutex` before accessing `callbackQueue`
- Use `std::unique_ptr<Callback>` for queue items (never raw pointers)
- Callbacks execute on SourceMod's main thread via `SDK_OnGameFrame()`

### 5. Asio Namespace Aliasing
Code supports both Boost.Asio and standalone Asio via `#ifdef USE_BOOST_ASIO`:
```cpp
#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#endif
```
Always use `asio::` prefix (never `boost::asio::` directly). Error codes are `boost::system::error_code` even with standalone Asio.

## Building (CRITICAL: 32-bit vs 64-bit)

### Standard 64-bit Build (Development/Testing)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_TLS=ON -DENABLE_IPV6=ON -DBUILD_TESTS=ON
cmake --build . -j$(nproc)
ctest --output-on-failure  # Run tests
```

### 32-bit Build (REQUIRED for SourceMod game servers)
Most Source engine servers run 32-bit SourceMod. **ALWAYS build 32-bit for production:**
```bash
# Install dependencies FIRST (Ubuntu/Debian)
sudo dpkg --add-architecture i386 && sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib \
    libboost-system1.83.0:i386 libboost-thread1.83.0:i386 libboost1.83-dev:i386

# Clean and build
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32" \
    -DCMAKE_EXE_LINKER_FLAGS="-m32" -DCMAKE_SHARED_LINKER_FLAGS="-m32" \
    -DENABLE_IPV6=ON -DENABLE_TLS=OFF
cmake --build .

# VERIFY architecture before deploying
file socket.ext.so  # Must show "ELF 32-bit Intel 80386"
```

**Critical:** CMake handles versioned Boost libraries (e.g., `libboost_system.so.1.83.0`) automatically for 32-bit. See `CMakeLists.txt` lines 30-65 for library search logic.

## File Structure & Where to Make Changes

**Core Implementation Files (root directory):**
- `Socket.{h,cpp}` (844 lines) - TCP/UDP socket operations, all CRUD methods
- `SocketHandler.{h,cpp}` (146 lines) - Socket factory, I/O service management
- `Callback.{h,cpp}` (189 lines) - Type-safe callback data storage (std::variant)
- `CallbackHandler.{h,cpp}` (61 lines) - Thread-safe callback queue
- `Extension.cpp` (445 lines) - SourceMod bridge, native function bindings
- `Define.h` (99 lines) - Enums, constants, type definitions

**Configuration:**
- `CMakeLists.txt` - Build system (handles 32/64-bit, Boost detection, SDK paths)
- `.clang-format` - Code style (run `clang-format -i *.cpp *.h` before committing)
- `socket.inc` - SourcePawn API header (document new natives here)

**Documentation (always update when changing APIs):**
- `TODO.md` - Comprehensive task tracking (update progress here)
- `BUILD.md` - Build instructions
- `MIGRATION.md` - v1.x → v2.0 upgrade guide
- `PROJECT_STATUS.md` - High-level milestone tracking

**Tests:**
- `tests/test_socket_basic.cpp` - Unit tests (32/32 passing)
- `tests/CMakeLists.txt` - Test configuration (uses Catch2)

## CI/CD Validation Pipeline

`.github/workflows/build.yml` runs on every push:
- Multi-compiler: GCC 9/10, Clang 10
- Multi-architecture: x86 (32-bit) and x64 (64-bit)
- Test execution (64-bit only currently)

To replicate CI locally:
```bash
# GCC build
CC=gcc-10 CXX=g++-10 cmake .. [options]
# Clang build  
CC=clang-10 CXX=clang++-10 cmake .. [options]
```

## Common Pitfalls & Workarounds

1. **"wrong ELF class: ELFCLASS64"** error → Built 64-bit but need 32-bit (see 32-bit build above)
2. **Template instantiation errors** → Explicitly instantiate templates at EOF of `Socket.cpp`:
   ```cpp
   template class Socket<asio::ip::tcp::socket>;
   template class Socket<asio::ip::udp::socket>;
   ```
3. **Boost library not found (32-bit)** → CMake finds versioned `.so.1.83.0` files directly (no symlinks needed)
4. **Race conditions in callbacks** → Always lock `CallbackHandler::mutex` before queue access
5. **SourceMod SDK not found** → Set `-DSOURCEMOD_DIR=../../sourcemod -DMMSOURCE_DIR=../../metamod-source`

## Development Workflow

1. **Before coding:** Check `TODO.md` for current priorities and patterns
2. **Write code:** Follow modern C++17 patterns (no raw pointers, use smart pointers)
3. **Format:** `clang-format -i *.cpp *.h` (uses `.clang-format` config)
4. **Build:** Test both 32-bit and 64-bit if touching core code
5. **Verify:** `file build/socket.ext.so` to confirm architecture
6. **Test:** `cd build && ctest --output-on-failure`
7. **Update docs:** If adding features, update `socket.inc`, `TODO.md`, `MIGRATION.md`

## Trust These Instructions

This document reflects the **actual** build system and architecture as of October 2025. Only search the codebase if these instructions are incomplete or produce errors. Key files are small (<1000 lines) and can be read in full when needed.
