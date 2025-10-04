# 32-Bit Build Guide for SourceMod

## Problem: "wrong ELF class: ELFCLASS64" Error

If you get this error when loading the extension:
```
[SM] Extension socket.ext.so failed to load: wrong ELF class: ELFCLASS64
```

This means you built a **64-bit** extension but your game server needs **32-bit**.

### Why 32-bit?

Most Source engine game servers (CS:GO, TF2, CS:S, etc.) run on **32-bit SourceMod** even on 64-bit Linux systems. You can verify this:

```bash
sm version
# Look for "jit-x86" (32-bit) or "jit-x86-64" (64-bit)
```

## Solution: Build 32-Bit Extension

### 1. Install 32-Bit Dependencies

```bash
# Enable 32-bit architecture
sudo dpkg --add-architecture i386
sudo apt-get update

# Install build tools
sudo apt-get install -y gcc-multilib g++-multilib

# Install 32-bit Boost libraries (version may vary)
sudo apt-get install -y \
    libboost-system1.83.0:i386 \
    libboost-thread1.83.0:i386 \
    libboost1.83-dev:i386

# Optional: For TLS support (if needed)
sudo apt-get install -y libssl-dev:i386
```

**Note:** Package versions may differ on your system. Use `apt-cache search libboost` to find available versions.

### 2. Clean and Reconfigure

```bash
cd sm-ext-socket
rm -rf build
mkdir build
cd build

# Configure for 32-bit
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-m32" \
    -DCMAKE_CXX_FLAGS="-m32" \
    -DCMAKE_EXE_LINKER_FLAGS="-m32" \
    -DCMAKE_SHARED_LINKER_FLAGS="-m32" \
    -DENABLE_TLS=OFF \
    -DENABLE_IPV6=ON
```

**Important:** TLS is disabled by default for 32-bit unless you installed `libssl-dev:i386`.

### 3. Build

```bash
make -j$(nproc)
```

### 4. Verify Architecture

```bash
file socket.ext.so
```

**Expected output:**
```
socket.ext.so: ELF 32-bit LSB shared object, Intel 80386, version 1 (GNU/Linux)
```

**Wrong output (don't use this):**
```
socket.ext.so: ELF 64-bit LSB shared object, x86-64, version 1 (GNU/Linux)
```

### 5. Install to Game Server

```bash
cp socket.ext.so /path/to/csgo/csgo/addons/sourcemod/extensions/
```

### 6. Test

Restart your server and check:
```
sm exts list
```

You should see `socket.ext.so` loaded successfully.

## Troubleshooting

### "Could not find Boost libraries"

The 32-bit Boost packages don't create symlinks automatically. Our CMakeLists.txt handles this by finding versioned `.so` files directly. If you still have issues:

```bash
# Check if libraries exist
ls -la /usr/lib/i386-linux-gnu/libboost_*.so*

# If you see .so.1.83.0 files but no .so symlinks, that's normal
# CMake will find them automatically
```

### "TLS support required"

If you need TLS/SSL support:

```bash
sudo apt-get install -y libssl-dev:i386
```

Then rebuild with `-DENABLE_TLS=ON`.

### "Still getting ELFCLASS64 error"

Make sure you're copying the **newly built** 32-bit version:

```bash
# Check what you're about to copy
file build/socket.ext.so

# Should show "32-bit" not "64-bit"
```

## Build Comparison

| Architecture | File Size | ELF Class | Use Case |
|-------------|-----------|-----------|----------|
| 32-bit | 2.1 MB | ELFCLASS32 | SourceMod game servers (CS:GO, TF2, etc.) |
| 64-bit | 2.2 MB | ELFCLASS64 | Modern 64-bit servers (CS2, future games) |

## Quick Reference

```bash
# Install dependencies (Ubuntu/Debian)
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib \
    libboost-system1.83.0:i386 \
    libboost-thread1.83.0:i386 \
    libboost1.83-dev:i386

# Build 32-bit
cd sm-ext-socket
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32" \
    -DCMAKE_EXE_LINKER_FLAGS="-m32" -DCMAKE_SHARED_LINKER_FLAGS="-m32" \
    -DENABLE_TLS=OFF
make -j$(nproc)

# Verify
file socket.ext.so  # Must show "32-bit"

# Install
cp socket.ext.so /path/to/gameserver/addons/sourcemod/extensions/
```

## Technical Details

The v2.0 modernization includes automatic 32-bit support:

- **Auto-detection:** CMake detects `-m32` flags and configures accordingly
- **Versioned libraries:** Handles `libboost_system.so.1.83.0` directly
- **Library paths:** Automatically sets `/usr/lib/i386-linux-gnu`
- **Tests:** Unit tests run on both 32-bit and 64-bit (32/32 passing)

For more information, see [BUILD.md](BUILD.md).
