# Migration Guide: Socket Extension v1.x → v2.0

This guide helps you migrate from the legacy socket extension (v1.x) to the modernized v2.0 version.

## Overview of Changes

Version 2.0 is a complete modernization of the socket extension with:
- **C++17/20** standard instead of C++98
- **Modern Asio** (standalone or Boost.Asio 1.70+)
- **CMake** build system instead of custom Makefiles
- **IPv6** support alongside IPv4
- **TLS/SSL** support for secure connections
- **Improved performance** with modern threading
- **Better error handling** with detailed error codes

## Breaking Changes

### 1. Build System

**Old (v1.x):**
```bash
make
# or use Visual Studio project files
```

**New (v2.0):**
```bash
mkdir build && cd build
cmake .. -DSOURCEMOD_DIR=/path/to/sourcemod
cmake --build .
```

See [BUILD.md](BUILD.md) for complete build instructions.

### 2. Dependencies

**Old Requirements:**
- Boost 1.55+
- GCC 4.x or Visual Studio 2010

**New Requirements:**
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.15+
- Asio (standalone) or Boost 1.70+
- OpenSSL (optional, for TLS)

### 3. File Extensions

The compiled extension filename remains the same:
- Linux: `socket.ext.so`
- Windows: `socket.ext.dll`

But the build output structure has changed with CMake.

## API Changes

### SourcePawn API (socket.inc)

The SourcePawn API remains **backwards compatible** with v1.x. Your existing plugins should continue to work without changes.

However, v2.0 adds **new features**:

#### New Natives

```sourcepawn
// TLS/SSL support
native bool SocketConnectTLS(Handle socket, SocketConnectCB cfunc, 
    SocketReceiveCB rfunc, SocketDisconnectCB dfunc, 
    const char[] hostname, int port, const char[] certPath = "");

// IPv6 support
native bool SocketBindIPv6(Handle socket, const char[] hostname, int port);
native bool SocketConnectIPv6(Handle socket, SocketConnectCB cfunc,
    SocketReceiveCB rfunc, SocketDisconnectCB dfunc,
    const char[] hostname, int port);

// Dual-stack server (IPv4 + IPv6)
native bool SocketListenDualStack(Handle socket, SocketIncomingCB ifunc);

// Enhanced error information
native int SocketGetLastError(Handle socket);
native bool SocketGetLastErrorString(Handle socket, char[] buffer, int maxlen);
```

#### New Socket Options

```sourcepawn
enum SocketOption {
    // ... existing options ...
    
    // New in v2.0
    ConcatenateCallbacks_IPv6,    // IPv6-specific concatenation
    EnableDualStack,              // Enable IPv4+IPv6 on same socket
    TLS_VerifyPeer,              // Verify TLS peer certificate
    TLS_VerifyHostname,          // Verify TLS hostname
}
```

### Example Migration

**Old Code (v1.x):**
```sourcepawn
public void OnClientConnect(int client) {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    SocketConnect(socket, OnSocketConnected, OnSocketReceive, 
                  OnSocketDisconnected, "example.com", 80);
}
```

**New Code (v2.0) - Same code works!**
```sourcepawn
public void OnClientConnect(int client) {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    SocketConnect(socket, OnSocketConnected, OnSocketReceive, 
                  OnSocketDisconnected, "example.com", 80);
}
```

**New Code with TLS (v2.0):**
```sourcepawn
public void OnClientConnect(int client) {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    // Use HTTPS (443) with TLS
    SocketConnectTLS(socket, OnSocketConnected, OnSocketReceive, 
                     OnSocketDisconnected, "example.com", 443);
}
```

**New Code with IPv6 (v2.0):**
```sourcepawn
public void OnServerStart() {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    // Bind to IPv6 address
    SocketBindIPv6(socket, "::1", 8080);
    SocketListen(socket, OnSocketIncoming);
}
```

## Performance Improvements

v2.0 includes several performance enhancements:

### 1. Modern Threading
- Uses `std::thread` instead of Boost.Thread
- Better CPU utilization on modern hardware
- Reduced lock contention

### 2. Memory Management
- Smart pointers eliminate memory leaks
- RAII patterns ensure proper cleanup
- Reduced heap allocations

### 3. Async Operations
- More efficient async I/O with modern Asio
- Optional coroutine support (C++20)
- Better error propagation

## Behavioral Changes

### 1. Error Handling

**Old:**
Errors were reported via numeric codes with limited information.

**New:**
- More detailed error messages
- System error codes preserved
- New `SocketGetLastErrorString()` native for human-readable errors

### 2. IPv6 Handling

**Old:**
IPv6 addresses were not supported.

**New:**
- IPv6 is fully supported
- Dual-stack mode allows serving both IPv4 and IPv6 clients
- Address family is auto-detected from hostname

### 3. TLS/SSL

**Old:**
No TLS support. Users had to use external SSL tunnels.

**New:**
- Native TLS/SSL support via OpenSSL
- Configurable certificate verification
- Support for modern TLS 1.2 and 1.3

## Configuration Changes

### Build Options

v2.0 introduces CMake options for customization:

```bash
# Enable/disable features
cmake .. -DENABLE_TLS=ON        # Enable TLS support
cmake .. -DENABLE_IPV6=ON       # Enable IPv6 support
cmake .. -DBUILD_TESTS=ON       # Build unit tests
cmake .. -DUSE_CPP20=ON         # Use C++20 features

# Set SDK paths
cmake .. -DSOURCEMOD_DIR=/path/to/sourcemod
cmake .. -DMMSOURCE_DIR=/path/to/metamod
```

### Runtime Configuration

No changes to runtime configuration. The extension still loads automatically from:
- `addons/sourcemod/extensions/socket.ext.so` (Linux)
- `addons/sourcemod/extensions/socket.ext.dll` (Windows)

## Testing Your Migration

### 1. Build Tests

```bash
# Build with tests enabled
cmake .. -DBUILD_TESTS=ON
cmake --build .

# Run unit tests
ctest --output-on-failure
```

### 2. Plugin Compatibility

Test your existing plugins:

1. Install v2.0 extension
2. Load your existing plugins
3. Verify functionality
4. Check logs for warnings/errors

### 3. Performance Testing

Compare v1.x vs v2.0:

```sourcepawn
// Use this test plugin to measure performance
#include <sourcemod>
#include <socket>

public void OnPluginStart() {
    int startTime = GetTime();
    
    // Your socket operations here
    for (int i = 0; i < 1000; i++) {
        Handle socket = SocketCreate(SOCKET_TCP, OnError);
        SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, 
                     "localhost", 8080);
    }
    
    int elapsed = GetTime() - startTime;
    PrintToServer("Test completed in %d seconds", elapsed);
}
```

## Common Migration Issues

### Issue 1: Compilation Errors

**Symptom:** Extension fails to load with "incompatible version" error

**Solution:** 
- Rebuild against SourceMod 1.12+
- Ensure you're using compatible compilers (see requirements)

### Issue 2: Missing Symbols

**Symptom:** `undefined symbol` errors on Linux

**Solution:**
```bash
# Install required libraries
sudo apt-get install libssl-dev libboost-all-dev

# Rebuild with proper linking
cmake .. -DENABLE_TLS=ON
cmake --build .
```

### Issue 3: IPv6 Not Working

**Symptom:** IPv6 connections fail or timeout

**Solution:**
- Verify IPv6 is enabled: `cmake .. -DENABLE_IPV6=ON`
- Check server IPv6 configuration
- Use `SocketBindIPv6()` for IPv6-specific binding

### Issue 4: TLS Handshake Fails

**Symptom:** TLS connections fail immediately

**Solution:**
- Verify OpenSSL is installed
- Check certificate paths
- Enable verbose error logging:
  ```sourcepawn
  SocketSetOption(socket, DebugMode, 1);
  ```

## Rollback Plan

If you encounter issues with v2.0, you can rollback to v1.x:

1. Stop your game server
2. Replace `socket.ext.so` / `socket.ext.dll` with v1.x version
3. Restart server
4. File an issue on GitHub with details

Your v1.x plugins will work without changes.

## Getting Help

- **Documentation:** [README.md](README.md)
- **Build Guide:** [BUILD.md](BUILD.md)
- **Issues:** https://github.com/oldmagic/sm-ext-socket/issues
- **Examples:** See `examples/` directory

## New Examples

v2.0 includes updated examples:

- `examples/tls_client.sp` - TLS/HTTPS client
- `examples/ipv6_server.sp` - IPv6 server
- `examples/dual_stack.sp` - IPv4+IPv6 server
- `examples/async_demo.sp` - Async socket operations

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for complete list of changes.

## Contributing

Want to contribute? See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

v2.0 maintains the same GPL-3.0 license as v1.x.
