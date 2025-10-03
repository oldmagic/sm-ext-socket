# Building Socket Extension v2.0

This document provides detailed instructions for building the SourceMod Socket Extension from source.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Linux Build](#linux-build)
- [Windows Build](#windows-build)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### All Platforms
- CMake 3.15 or higher
- C++17 compatible compiler (C++20 recommended)
- Git

### Linux
- GCC 9+ or Clang 10+
- Build essentials: `build-essential cmake ninja-build`
- Boost libraries (1.70+): `libboost-all-dev`
- OpenSSL (for TLS): `libssl-dev`
- For 32-bit builds: `gcc-multilib g++-multilib`

### Windows
- Visual Studio 2019 or 2022
- vcpkg (for dependency management)
- Windows SDK

### SourceMod Dependencies
- SourceMod SDK (1.12 or higher)
- MetaMod:Source (1.11 or higher)

## Quick Start

```bash
# Clone the repository
git clone https://github.com/oldmagic/sm-ext-socket.git
cd sm-ext-socket

# Clone SourceMod SDK (adjust paths as needed)
cd ..
git clone https://github.com/alliedmodders/sourcemod.git
git clone https://github.com/alliedmodders/metamod-source.git
cd sm-ext-socket

# Configure and build
mkdir build && cd build
cmake .. -DSOURCEMOD_DIR=../../sourcemod -DMMSOURCE_DIR=../../metamod-source
cmake --build . --config Release

# Install (optional)
sudo cmake --install .
```

## Linux Build

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git \
    libboost-all-dev libssl-dev
```

### For 32-bit Builds

SourceMod typically uses 32-bit builds on Linux:

```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib \
    libboost-all-dev:i386 libssl-dev:i386
```

### Configure and Build

```bash
mkdir build && cd build

# 64-bit build
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_TLS=ON \
    -DENABLE_IPV6=ON \
    -DSOURCEMOD_DIR=/path/to/sourcemod \
    -DMMSOURCE_DIR=/path/to/metamod-source

# 32-bit build (for SourceMod compatibility)
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32 \
    -DENABLE_TLS=ON \
    -DENABLE_IPV6=ON \
    -DSOURCEMOD_DIR=/path/to/sourcemod \
    -DMMSOURCE_DIR=/path/to/metamod-source

# Build
cmake --build .

# Run tests (optional)
ctest --output-on-failure
```

### Output
The compiled extension will be at: `build/socket.ext.so`

## Windows Build

### Install Dependencies

1. Install Visual Studio 2019/2022 with C++ workload
2. Install vcpkg:
   ```cmd
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

3. Install required libraries:
   ```cmd
   vcpkg install boost-asio:x86-windows openssl:x86-windows
   vcpkg install boost-asio:x64-windows openssl:x64-windows
   ```

### Configure and Build

Using CMake GUI or command line:

```cmd
mkdir build
cd build

cmake .. -A Win32 ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DENABLE_TLS=ON ^
    -DENABLE_IPV6=ON ^
    -DSOURCEMOD_DIR=C:\path\to\sourcemod ^
    -DMMSOURCE_DIR=C:\path\to\metamod-source

cmake --build . --config Release
```

### Output
The compiled extension will be at: `build\Release\socket.ext.dll`

## Build Options

Configure the build using CMake options:

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_TLS` | ON | Enable TLS/SSL support (requires OpenSSL) |
| `ENABLE_IPV6` | ON | Enable IPv6 support |
| `BUILD_TESTS` | ON | Build unit tests (requires Catch2) |
| `USE_CPP20` | OFF | Use C++20 standard (requires compiler support) |
| `CMAKE_BUILD_TYPE` | Release | Build type: Debug, Release, RelWithDebInfo |
| `SOURCEMOD_DIR` | ../sourcemod | Path to SourceMod SDK |
| `MMSOURCE_DIR` | ../metamod-source | Path to MetaMod:Source |

Example with custom options:

```bash
cmake .. \
    -DENABLE_TLS=OFF \
    -DENABLE_IPV6=ON \
    -DBUILD_TESTS=OFF \
    -DUSE_CPP20=ON \
    -DCMAKE_BUILD_TYPE=Debug
```

## Installation

### Manual Installation

Copy the built files to your game server:

```bash
# Linux
cp build/socket.ext.so /path/to/gameserver/addons/sourcemod/extensions/
cp socket.inc /path/to/gameserver/addons/sourcemod/scripting/include/

# Windows
copy build\Release\socket.ext.dll \path\to\gameserver\addons\sourcemod\extensions\
copy socket.inc \path\to\gameserver\addons\sourcemod\scripting\include\
```

### Using CMake Install

```bash
cmake --install . --prefix /path/to/gameserver
```

## Troubleshooting

### SourceMod SDK Not Found

If CMake cannot find the SourceMod SDK:

```bash
cmake .. -DSOURCEMOD_DIR=/absolute/path/to/sourcemod \
         -DMMSOURCE_DIR=/absolute/path/to/metamod-source
```

### Boost Not Found

**Linux:**
```bash
sudo apt-get install libboost-all-dev
```

**Windows:**
```cmd
vcpkg install boost-asio:x86-windows
```

### OpenSSL Not Found

**Linux:**
```bash
sudo apt-get install libssl-dev
```

**Windows:**
```cmd
vcpkg install openssl:x86-windows
```

### 32-bit Build Issues on Linux

Ensure you have 32-bit libraries installed:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install gcc-multilib g++-multilib
```

### Compiler Too Old

This extension requires C++17 support:
- GCC 9 or higher
- Clang 10 or higher
- MSVC 2019 or higher

## Development Builds

For development and debugging:

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DBUILD_TESTS=ON \
         -DSOURCEMOD_DIR=/path/to/sourcemod \
         -DMMSOURCE_DIR=/path/to/metamod-source
cmake --build .

# Run tests
ctest --output-on-failure --verbose
```

## Cross-Compilation

### Linux to Windows (MinGW)

```bash
# Install MinGW
sudo apt-get install mingw-w64

# Configure
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64.cmake \
         -DSOURCEMOD_DIR=/path/to/sourcemod \
         -DMMSOURCE_DIR=/path/to/metamod-source

# Build
cmake --build .
```

## CI/CD Builds

This project uses GitHub Actions for automated builds. See `.github/workflows/build.yml` for the complete CI configuration.

Builds are automatically triggered on:
- Push to master/develop
- Pull requests
- Release tags

Artifacts are available for download from the Actions tab.

## Additional Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [SourceMod Extension Development](https://wiki.alliedmods.net/SourceMod_SDK)
- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/libs/asio/)
- [Project README](README.md)
