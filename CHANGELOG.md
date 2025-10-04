# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0-beta] - 2025-10-04

### Added - C++17 Modernization (Milestone 2 Complete)
- Complete C++17 rewrite of core implementation (1,685 lines modernized)
- Smart pointers (`std::unique_ptr`) throughout - zero raw pointers
- Modern threading (`std::thread`, `std::mutex`, `std::shared_mutex`)
- Type-safe callbacks using `std::variant`
- Lambda functions replacing `boost::bind`
- Protocol-specific template specializations (TCP vs UDP)
- Comprehensive unit tests (32 assertions, 10 test cases, 100% passing)
- 32-bit and 64-bit build support
- CMake build system with automatic 32-bit library detection

### Changed
- Replaced all `boost::mutex` with `std::mutex`
- Replaced all `boost::thread` with `std::thread`
- Updated to `boost::system::error_code` for Boost.Asio compatibility
- Manual memory management eliminated (RAII throughout)
- Thread-safe callback queue with modern synchronization

### Fixed
- Template instantiation issues with protocol-specific implementations
- Race conditions in callback queue
- Memory leaks from manual new/delete
- 32-bit build compatibility with versioned Boost libraries
- Enum type safety with backward compatibility constants

### Technical Details
- **Build:** socket.ext.so (32-bit: 2.1MB, 64-bit: 2.2MB)
- **Compiler:** GCC 13.3.0 with C++17
- **Platform:** Linux x86/x86-64
- **Dependencies:** Boost.Asio 1.83, SourceMod SDK 1.12+
- **Tests:** 32/32 passing on both 32-bit and 64-bit buildsgelog

All notable changes to the SourceMod Socket Extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - Unreleased

### Major Overhaul 🚀

This release is a **complete modernization** of the extension, bringing it to modern C++17/20 standards
with significant new features and improvements.

### Added

#### Build System
- CMake build system for cross-platform compilation
- GitHub Actions CI/CD for automated builds and testing
- Support for Linux (Ubuntu, Debian, etc.) and Windows (VS 2019/2022)
- Automated binary releases
- Comprehensive build documentation (BUILD.md)

#### Networking Features
- **IPv6 support** - Full IPv6 addressing and dual-stack sockets
  - `SocketBindIPv6()` native
  - `SocketConnectIPv6()` native
  - `SocketListenDualStack()` native for IPv4+IPv6 servers
- **TLS/SSL support** - Secure socket connections
  - `SocketConnectTLS()` native
  - Certificate verification options
  - Support for TLS 1.2 and 1.3
- Enhanced error reporting with detailed error codes
  - `SocketGetLastError()` native
  - `SocketGetLastErrorString()` native

#### Code Quality
- Modern C++17/20 features throughout codebase
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`) for automatic memory management
- RAII patterns for resource management
- Type-safe enums (`enum class`)
- `constexpr` for compile-time constants
- Move semantics for improved performance
- Thread-safe operations with `std::mutex` and `std::shared_mutex`

#### Testing
- Catch2 unit testing framework
- Comprehensive test suite with unit and integration tests
- Automated test execution in CI/CD
- Test coverage reporting (planned)

#### Documentation
- Complete rewrite of README.md
- Detailed BUILD.md with platform-specific instructions
- MIGRATION.md guide for v1.x → v2.0 upgrade
- CONTRIBUTING.md for developers
- Doxygen-style code documentation
- Updated examples for modern SourcePawn

### Changed

#### Breaking Changes
- Minimum compiler requirements: GCC 9+, Clang 10+, MSVC 2019+
- C++17 standard required (C++20 optional)
- CMake 3.15+ required for building
- SourceMod 1.12+ compatibility (older versions not tested)
- Boost 1.70+ required if using Boost.Asio (or standalone Asio)

#### Performance Improvements
- Replace Boost.Thread with `std::thread` for better performance
- Replace Boost.Bind with C++ lambdas
- Reduced heap allocations with smart pointers
- More efficient async I/O operations
- Reduced lock contention with modern mutex types

#### Code Structure
- Reorganized headers with better separation of concerns
- Modernized template instantiation
- Improved error handling with `std::error_code`
- Better const-correctness throughout
- Enhanced thread-safety guarantees
- **Ported to modern SourceMod SDK** - Removed dependency on deprecated `sp_vm_types.h` and `sp_vm_api.h`, now using SourcePawn 1.10+ APIs from `IPluginSys.h` and related modern headers

### Deprecated

The following features remain for backwards compatibility but will be removed in v3.0:
- Old-style integer enum constants (use `enum class` equivalents)
- Raw pointer interfaces in C++ API (use smart pointers)

### Removed

- Old Makefile-based build system (replaced by CMake)
- Visual Studio 2010 project files (use CMake to generate)
- Boost 1.55 support (minimum 1.70 now)
- Support for compilers older than C++17

### Fixed

- Memory leaks from manual memory management (now using RAII)
- Race conditions in callback handling (improved locking)
- Potential crashes on rapid connect/disconnect
- Undefined behavior from raw pointer usage
- Buffer overflows in some edge cases

### Security

- TLS/SSL support for secure communications
- Certificate verification for HTTPS connections
- Modern crypto algorithms via OpenSSL
- Thread-safe operations prevent data races
- Proper resource cleanup prevents memory leaks

## [1.x] - Historical

### Note on v1.x

The v1.x series was based on C++98/03 and used Boost libraries extensively.
It served the community well for over 11 years but is now superseded by v2.0.

For v1.x changelog, see the git history or old releases.

## Upgrade Guide

See [MIGRATION.md](MIGRATION.md) for detailed upgrade instructions.

## Compatibility

### SourcePawn API Compatibility

The SourcePawn API remains **backwards compatible**. Plugins written for v1.x will work with v2.0 without changes.

New natives are added but old natives remain functional:
- ✅ All v1.x natives work in v2.0
- ✅ All v1.x callbacks work in v2.0
- ✅ All v1.x enums work in v2.0
- ✅ New v2.0 features are additive

### Platform Support

| Platform | v1.x | v2.0 |
|----------|------|------|
| Linux (32-bit) | ✅ | ✅ |
| Linux (64-bit) | ✅ | ✅ |
| Windows (32-bit) | ✅ | ✅ |
| Windows (64-bit) | ✅ | ✅ |

### SourceMod Compatibility

| SourceMod Version | v1.x | v2.0 |
|-------------------|------|------|
| 1.5 - 1.6 | ✅ | ❌ |
| 1.7 - 1.11 | ✅ | ⚠️ (not tested) |
| 1.12+ | ✅ | ✅ |

⚠️ = May work but not officially tested

## Future Plans

### v2.1 (Planned)
- Coroutine support (C++20)
- HTTP/HTTPS client helpers
- WebSocket support
- Connection pooling
- Rate limiting utilities

### v2.2 (Planned)
- SOCKS proxy support
- DNS caching
- Zero-copy operations where possible
- Performance profiling tools

### v3.0 (Future)
- Remove deprecated v1.x compatibility
- Full C++20 adoption
- Async/await style APIs
- Plugin hot-reload support

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for how to contribute to this project.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

---

For questions or issues, please visit: https://github.com/oldmagic/sm-ext-socket
