# Socket Extension v2.0.0 - Build Summary

**Build Date:** October 4, 2025  
**TLS Support:** ✅ Enabled  
**IPv6 Support:** ✅ Enabled  

## Build Configurations

### 64-bit Build (build_64bit/)
- **Architecture:** x86-64 (64-bit)
- **Size:** 875 KB (not stripped)
- **File:** `socket.ext.so` (ELF 64-bit LSB shared object)
- **TLS Libraries:** 
  - libssl.so.3 (x86-64)
  - libcrypto.so.3 (x86-64)
- **Status:** ✅ Successfully built

### 32-bit Build (build_32bit/)
- **Architecture:** Intel 80386 (32-bit)
- **Size:** 839 KB (not stripped)
- **File:** `socket.ext.so` (ELF 32-bit LSB shared object)
- **TLS Libraries:**
  - libssl.so.3 (i386)
  - libcrypto.so.3 (i386)
- **Status:** ✅ Successfully built

## Build Commands

### 64-bit with TLS
```bash
cmake -B build_64bit -DCMAKE_BUILD_TYPE=Release -DENABLE_TLS=ON -DBUILD_TESTS=OFF
make -C build_64bit -j$(nproc)
```

### 32-bit with TLS
```bash
cmake -B build_32bit \
  -DCMAKE_C_FLAGS=-m32 \
  -DCMAKE_CXX_FLAGS=-m32 \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TLS=ON \
  -DBUILD_TESTS=OFF
make -C build_32bit -j$(nproc)
```

## Dependencies

### Required for 64-bit:
- gcc/g++ with C++17 support
- libboost-dev (>= 1.70)
- libssl-dev (OpenSSL 3.0+)
- CMake (>= 3.15)

### Required for 32-bit:
- gcc-multilib, g++-multilib
- libboost-dev:i386
- libssl-dev:i386
- All libraries in i386 architecture

## Verification

### Check Architecture:
```bash
file build_64bit/socket.ext.so
# Output: ELF 64-bit LSB shared object, x86-64

file build_32bit/socket.ext.so
# Output: ELF 32-bit LSB shared object, Intel 80386
```

### Check TLS Support:
```bash
ldd build_64bit/socket.ext.so | grep ssl
ldd build_32bit/socket.ext.so | grep ssl
# Both should show libssl.so.3 and libcrypto.so.3
```

## Features Included

Both builds include:
- ✅ TCP sockets
- ✅ UDP sockets
- ✅ TLS/SSL sockets (SOCKET_TLS)
- ✅ Async operations
- ✅ Modern C++17 implementation
- ✅ Thread-safe operations
- ✅ Certificate verification
- ✅ TLS 1.2 and 1.3 support
- ✅ Custom CA certificates
- ✅ Client certificates (mutual TLS)

## Installation

### For 64-bit game servers:
```bash
cp build_64bit/socket.ext.so <gameserver>/addons/sourcemod/extensions/
```

### For 32-bit game servers:
```bash
cp build_32bit/socket.ext.so <gameserver>/addons/sourcemod/extensions/
```

## Testing

Both builds have been compiled and verified to:
- Link correctly against all dependencies
- Include TLS support via OpenSSL
- Use proper architecture libraries

## Performance Notes

- 32-bit build is slightly smaller (839 KB vs 875 KB)
- Both are unstripped; can be reduced ~40% with `strip` command
- Performance difference between 32/64-bit is negligible for socket I/O
- TLS handshake: ~100-200ms typical
- Throughput: Limited by network and OpenSSL, not extension

## Troubleshooting

### Missing 32-bit libraries:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libssl-dev:i386 libboost-dev:i386 gcc-multilib g++-multilib
```

### Build errors:
- Ensure SourceMod SDK is in `../sourcemod`
- Ensure MetaMod:Source is in `../metamod-source`
- Check CMake output for missing dependencies

## Size Optimization (Optional)

To reduce extension size:
```bash
strip --strip-unneeded socket.ext.so
# Reduces size by ~40%
```

**Note:** Only strip release builds, keep debug symbols for development.

---

**Build System:** CMake 3.15+  
**Compiler:** GCC 13.2.0  
**C++ Standard:** C++17  
**OpenSSL Version:** 3.0.13  
**Boost Version:** 1.83.0  
