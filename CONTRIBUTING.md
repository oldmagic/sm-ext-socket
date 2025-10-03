# Contributing to Socket Extension

Thank you for your interest in contributing to the SourceMod Socket Extension!

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Coding Standards](#coding-standards)
- [Documentation](#documentation)

## Code of Conduct

This project follows a code of conduct to ensure a welcoming environment:
- Be respectful and inclusive
- Provide constructive feedback
- Focus on what is best for the community
- Show empathy towards others

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/your-username/sm-ext-socket.git
   cd sm-ext-socket
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/oldmagic/sm-ext-socket.git
   ```
4. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.15+
- Git
- SourceMod SDK 1.12+
- MetaMod:Source 1.11+

### Building for Development

```bash
# Clone dependencies
cd ..
git clone https://github.com/alliedmodders/sourcemod.git
git clone https://github.com/alliedmodders/metamod-source.git
cd sm-ext-socket

# Configure with debug info and tests
mkdir build-dev && cd build-dev
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DENABLE_TLS=ON \
    -DENABLE_IPV6=ON \
    -DSOURCEMOD_DIR=../../sourcemod \
    -DMMSOURCE_DIR=../../metamod-source

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### IDE Setup

**Visual Studio Code:**
```json
{
    "cmake.configureSettings": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_TESTS": "ON",
        "SOURCEMOD_DIR": "${workspaceFolder}/../sourcemod",
        "MMSOURCE_DIR": "${workspaceFolder}/../metamod-source"
    }
}
```

**CLion / Visual Studio:**
- Open the project directory
- CMake will auto-configure
- Set CMake options in IDE settings

## Making Changes

### Branch Naming

Use descriptive branch names:
- `feature/add-tls-support` - New features
- `bugfix/fix-ipv6-binding` - Bug fixes
- `docs/update-readme` - Documentation
- `refactor/modernize-callbacks` - Code refactoring

### Commit Messages

Follow conventional commit format:

```
type(scope): subject

body

footer
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Build process or tooling

**Examples:**
```
feat(socket): add TLS/SSL support

Implement TLS support using OpenSSL and Asio SSL streams.
Adds new SocketConnectTLS() native for secure connections.

Closes #42
```

```
fix(callback): prevent memory leak in error handler

Use smart pointers to ensure proper cleanup when callbacks fail.

Fixes #38
```

### Code Style

We use `.clang-format` for consistent formatting:

```bash
# Format all C++ files
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Format specific file
clang-format -i Socket.cpp
```

### C++ Guidelines

Follow modern C++ best practices:

#### 1. Use Smart Pointers
```cpp
// Good
auto socket = std::make_unique<tcp::socket>(ioService);

// Bad
tcp::socket* socket = new tcp::socket(ioService);
```

#### 2. Use RAII
```cpp
// Good
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Critical section
} // Automatically unlocked

// Bad
mutex_.lock();
// Critical section
mutex_.unlock();
```

#### 3. Use Modern Error Handling
```cpp
// Good
std::error_code ec;
socket->connect(endpoint, ec);
if (ec) {
    // Handle error
}

// Bad
try {
    socket->connect(endpoint);
} catch (...) {
    // Generic catch
}
```

#### 4. Use `constexpr` and `const`
```cpp
// Good
constexpr int kMaxConnections = 100;
const std::string& GetName() const;

// Bad
#define MAX_CONNECTIONS 100
std::string& GetName();
```

#### 5. Prefer `std::string_view`
```cpp
// Good
bool Connect(std::string_view hostname, uint16_t port);

// Bad
bool Connect(const std::string& hostname, uint16_t port);
bool Connect(const char* hostname, uint16_t port);
```

## Testing

### Writing Tests

Tests use Catch2 framework. Place tests in `tests/`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "Socket.h"

TEST_CASE("Socket creation", "[socket]") {
    SECTION("TCP socket") {
        auto socket = CreateTCPSocket();
        REQUIRE(socket != nullptr);
        REQUIRE_FALSE(socket->IsOpen());
    }
    
    SECTION("UDP socket") {
        auto socket = CreateUDPSocket();
        REQUIRE(socket != nullptr);
    }
}

TEST_CASE("Socket connection", "[socket][integration]") {
    // Integration test
    auto socket = CreateTCPSocket();
    bool connected = socket->Connect("localhost", 8080);
    REQUIRE(connected);
}
```

### Running Tests

```bash
# Run all tests
ctest --output-on-failure

# Run specific test
ctest -R "Socket creation"

# Run with verbose output
ctest -V

# Run only integration tests
ctest -L integration
```

### Test Coverage

```bash
# Build with coverage
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build .

# Run tests
ctest

# Generate coverage report
gcov *.gcda
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Submitting Changes

### Pull Request Process

1. **Update your branch** with latest upstream:
   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

2. **Run tests**:
   ```bash
   cmake --build build-dev
   cd build-dev && ctest --output-on-failure
   ```

3. **Format code**:
   ```bash
   clang-format -i $(find . -name "*.cpp" -o -name "*.h")
   ```

4. **Commit changes**:
   ```bash
   git add .
   git commit -m "feat(socket): your feature description"
   ```

5. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create Pull Request** on GitHub

### Pull Request Checklist

- [ ] Code follows project style guidelines
- [ ] All tests pass
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] Commit messages follow convention
- [ ] No merge conflicts with master
- [ ] Code formatted with clang-format

### Review Process

1. Automated checks run (CI/CD)
2. Code review by maintainers
3. Address feedback and update PR
4. Approval and merge

## Documentation

### Code Documentation

Use Doxygen-style comments:

```cpp
/**
 * @brief Connect to remote host
 * 
 * Establishes a connection to the specified remote host and port.
 * This operation is asynchronous by default.
 * 
 * @param hostname Remote hostname or IP address
 * @param port Remote port number (1-65535)
 * @param async Perform operation asynchronously (default: true)
 * @return true if connection initiated successfully
 * 
 * @throws std::invalid_argument if hostname is empty
 * @throws std::out_of_range if port is invalid
 * 
 * @note This function returns immediately in async mode
 * @see Disconnect(), IsConnected()
 * 
 * @example
 * @code
 * Socket socket;
 * if (socket.Connect("example.com", 80)) {
 *     // Connection initiated
 * }
 * @endcode
 */
bool Connect(std::string_view hostname, uint16_t port, bool async = true);
```

### User Documentation

Update relevant docs when making changes:
- `README.md` - Main documentation
- `BUILD.md` - Build instructions
- `MIGRATION.md` - Migration guide
- `examples/` - Example code

## Questions?

- **General questions:** Open a GitHub Discussion
- **Bug reports:** Open a GitHub Issue
- **Security issues:** Email the maintainers directly

## License

By contributing, you agree that your contributions will be licensed under the GPL-3.0 License.

Thank you for contributing! 🎉
