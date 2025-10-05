# IPv6 Support Guide for SourceMod Socket Extension v2.0

**Version:** 2.0.0  
**Date:** October 5, 2025  
**Status:** ✅ Implemented and Tested

## Overview

The SourceMod Socket Extension v2.0 includes full support for IPv6 (Internet Protocol version 6) alongside traditional IPv4. The extension automatically handles both protocols transparently, making it easy to support modern IPv6-enabled networks while maintaining backward compatibility with IPv4.

## Table of Contents

1. [Quick Start](#quick-start)
2. [IPv6 Basics](#ipv6-basics)
3. [Configuration Options](#configuration-options)
4. [Usage Examples](#usage-examples)
5. [Dual-Stack Operation](#dual-stack-operation)
6. [Troubleshooting](#troubleshooting)
7. [Best Practices](#best-practices)

## Quick Start

### Basic IPv6 Connection

```sourcepawn
// The extension automatically supports IPv6 - no special code needed!
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "ipv6.google.com", 80);
```

### Force IPv6 Only

```sourcepawn
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, IPv6Only, 1);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "ipv6.example.com", 80);
```

### Prefer IPv6 Over IPv4

```sourcepawn
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, PreferIPv6, 1);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "dualstack.example.com", 80);
```

## IPv6 Basics

### What is IPv6?

IPv6 is the next generation Internet Protocol designed to replace IPv4. Key differences:

- **Address Length**: IPv6 uses 128-bit addresses vs IPv4's 32-bit
- **Address Format**: IPv6 uses hexadecimal notation (e.g., `2001:db8::1`)
- **Address Space**: IPv6 provides 340 undecillion addresses
- **Features**: Built-in security (IPsec), no NAT required, better routing

### IPv6 Address Format

```
Full format:     2001:0db8:0000:0000:0000:0000:0000:0001
Compressed:      2001:db8::1
IPv4-mapped:     ::ffff:192.0.2.1
```

**Compression Rules:**
- Leading zeros in each group can be omitted: `0001` → `1`
- Consecutive groups of zeros can be replaced with `::` (once per address)

### Special IPv6 Addresses

| Address | Meaning |
|---------|---------|
| `::` | All zeros (any address) |
| `::1` | Loopback (localhost) |
| `::/0` | Default route (all addresses) |
| `fe80::/10` | Link-local addresses |
| `fc00::/7` | Unique local addresses (private) |
| `2000::/3` | Global unicast addresses |

## Configuration Options

The extension provides three IPv6-related socket options:

### IPv6Only

**Type:** Boolean  
**Purpose:** Enable IPv6-only mode, rejecting IPv4-mapped IPv6 addresses

```sourcepawn
SocketSetOption(socket, IPv6Only, 1);  // Enable
SocketSetOption(socket, IPv6Only, 0);  // Disable (default)
```

**When to use:**
- You want to ensure only pure IPv6 connections
- Testing IPv6-specific functionality
- Server is IPv6-only environment

### PreferIPv6

**Type:** Boolean  
**Purpose:** Prefer IPv6 addresses when both IPv4 and IPv6 are available

```sourcepawn
SocketSetOption(socket, PreferIPv6, 1);  // Prefer IPv6
SocketSetOption(socket, PreferIPv6, 0);  // No preference (default)
```

**When to use:**
- You want to use IPv6 whenever possible
- Testing IPv6 while maintaining IPv4 fallback
- Optimizing for IPv6-native networks

**Note:** Mutually exclusive with `PreferIPv4`

### PreferIPv4

**Type:** Boolean  
**Purpose:** Prefer IPv4 addresses when both IPv4 and IPv6 are available

```sourcepawn
SocketSetOption(socket, PreferIPv4, 1);  // Prefer IPv4
SocketSetOption(socket, PreferIPv4, 0);  // No preference (default)
```

**When to use:**
- You encounter IPv6 connectivity issues
- IPv4 provides better performance in your environment
- Maintaining compatibility with IPv4-only services

**Note:** Mutually exclusive with `PreferIPv6`

## Usage Examples

### Example 1: Default Dual-Stack Connection

```sourcepawn
#include <sourcemod>
#include <socket>

public void OnPluginStart() {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    // No IPv6 options needed - works automatically!
    SocketConnect(socket, OnSocketConnected, OnSocketReceive, 
                  OnSocketDisconnected, "example.com", 80);
}

public void OnSocketConnected(Handle socket, any arg) {
    PrintToServer("Connected to server (IPv4 or IPv6)");
    SocketSend(socket, "GET / HTTP/1.0\r\n\r\n");
}

public void OnSocketReceive(Handle socket, const char[] receiveData, 
                            const int dataSize, any arg) {
    PrintToServer("Received %d bytes", dataSize);
}

public void OnSocketDisconnected(Handle socket, any arg) {
    PrintToServer("Disconnected");
    CloseHandle(socket);
}

public void OnSocketError(Handle socket, const int errorType, 
                          const int errorNum, any arg) {
    PrintToServer("Socket error: %d (errno: %d)", errorType, errorNum);
    CloseHandle(socket);
}
```

### Example 2: IPv6-Only Connection

```sourcepawn
public void ConnectIPv6Only() {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    
    // Force IPv6-only mode
    SocketSetOption(socket, IPv6Only, 1);
    
    // Connect to IPv6 literal address
    SocketConnect(socket, OnSocketConnected, OnSocketReceive,
                  OnSocketDisconnected, "2001:4860:4860::8888", 53);
}
```

### Example 3: IPv6 with TLS

```sourcepawn
public void ConnectIPv6TLS() {
    Handle socket = SocketCreate(SOCKET_TLS, OnSocketError);
    
    // Prefer IPv6 but allow IPv4 fallback
    SocketSetOption(socket, PreferIPv6, 1);
    
    // Enable certificate verification
    SocketSetTLSOption(socket, TLSVerifyPeer, 1);
    SocketSetTLSOption(socket, TLSVerifyHost, 1);
    
    // Connect with TLS
    SocketConnectTLS(socket, OnSocketConnected, OnSocketReceive,
                     OnSocketDisconnected, "ipv6.google.com", 443);
}
```

### Example 4: UDP with IPv6

```sourcepawn
public void SendUDPIPv6() {
    Handle socket = SocketCreate(SOCKET_UDP, OnSocketError);
    
    // Prefer IPv6
    SocketSetOption(socket, PreferIPv6, 1);
    
    // Bind to any IPv6 address
    SocketBind(socket, "::", 0);  // :: means all IPv6 interfaces
    
    // Send data
    SocketSendTo(socket, "Hello IPv6!", -1, "2001:db8::1", 12345);
    
    CloseHandle(socket);
}
```

### Example 5: Listening on IPv6

```sourcepawn
public void ListenIPv6() {
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    
    // Enable IPv6
    SocketSetOption(socket, PreferIPv6, 1);
    
    // Bind to all IPv6 interfaces on port 8080
    SocketBind(socket, "::", 8080);
    
    // Listen for incoming connections
    SocketListen(socket, OnSocketIncoming);
}

public void OnSocketIncoming(Handle socket, Handle newSocket, 
                             const char[] remoteIP, int remotePort, any arg) {
    PrintToServer("Incoming connection from [%s]:%d", remoteIP, remotePort);
    
    // Set up callbacks for new socket
    SocketSetReceiveCallback(newSocket, OnSocketReceive);
    SocketSetDisconnectCallback(newSocket, OnSocketDisconnected);
    SocketSetErrorCallback(newSocket, OnSocketError);
    
    SocketSend(newSocket, "Welcome!\n");
}
```

## Dual-Stack Operation

### What is Dual-Stack?

Dual-stack means the socket can handle both IPv4 and IPv6 connections simultaneously. The extension implements dual-stack automatically.

### Default Behavior

By default, when you connect to a hostname:

1. The extension resolves both IPv4 (A records) and IPv6 (AAAA records)
2. The first successful connection is used
3. No explicit configuration needed

```sourcepawn
// This automatically works with both IPv4 and IPv6
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "example.com", 80);
```

### Controlling Protocol Preference

```sourcepawn
// Prefer IPv6, but use IPv4 if IPv6 fails or unavailable
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, PreferIPv6, 1);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "example.com", 80);

// Prefer IPv4, but use IPv6 if IPv4 fails or unavailable
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, PreferIPv4, 1);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "example.com", 80);
```

### IPv4-Mapped IPv6 Addresses

The extension handles IPv4-mapped IPv6 addresses automatically:

- Format: `::ffff:192.0.2.1` (IPv4 `192.0.2.1` mapped to IPv6)
- Used when IPv6 socket accepts IPv4 connections
- Transparent to your code

To disable IPv4-mapped addresses:
```sourcepawn
SocketSetOption(socket, IPv6Only, 1);
```

## Troubleshooting

### Issue: "NO_HOST" Error

**Symptoms:** Connection fails with `NO_HOST` error

**Possible Causes:**
1. Hostname doesn't resolve to IP address
2. IPv6Only is enabled but host has no IPv6 address
3. DNS resolution failure

**Solutions:**
```sourcepawn
// Try with IPv4 preference
SocketSetOption(socket, PreferIPv4, 1);

// Or disable IPv6-only mode
SocketSetOption(socket, IPv6Only, 0);

// Test with known IPv6 address
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "2606:2800:220:1:248:1893:25c8:1946", 80);
```

### Issue: IPv6 Address Not Working

**Check:**
1. Server has IPv6 connectivity (`ping6 ipv6.google.com`)
2. Firewall allows IPv6 traffic
3. DNS resolves AAAA records (`dig AAAA example.com`)

**Test:**
```sourcepawn
// Test with Google's public IPv6 DNS
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, IPv6Only, 1);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "2001:4860:4860::8888", 53);
```

### Issue: Mixed IPv4/IPv6 Environment

**Problem:** Some clients use IPv4, others use IPv6

**Solution:** Use default dual-stack mode (no options needed)
```sourcepawn
// Works for both IPv4 and IPv6 clients
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketBind(socket, "0.0.0.0", 8080);  // IPv4
// Or
SocketBind(socket, "::", 8080);  // IPv6 (may accept IPv4-mapped)
SocketListen(socket, OnIncoming);
```

### Debug IPv6 Issues

```sourcepawn
public void DebugConnection(const char[] hostname) {
    PrintToServer("Testing connection to: %s", hostname);
    
    // Test IPv4
    Handle socket4 = SocketCreate(SOCKET_TCP, OnError);
    SocketSetOption(socket4, PreferIPv4, 1);
    SocketConnect(socket4, OnConnect, OnReceive, OnDisconnect, hostname, 80);
    
    // Test IPv6
    Handle socket6 = SocketCreate(SOCKET_TCP, OnError);
    SocketSetOption(socket6, PreferIPv6, 1);
    SocketConnect(socket6, OnConnect, OnReceive, OnDisconnect, hostname, 80);
}
```

## Best Practices

### 1. Use Default Dual-Stack Mode

**Don't** force IPv4 or IPv6 unless you have a specific reason:
```sourcepawn
// Good: Works with both protocols
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "example.com", 80);
```

### 2. Set Preferences Early

Set IPv6 options **before** connecting:
```sourcepawn
// Correct order
Handle socket = SocketCreate(SOCKET_TCP, OnError);
SocketSetOption(socket, PreferIPv6, 1);  // Set first
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "example.com", 80);
```

### 3. Handle Both Protocols in Listeners

When listening for connections, be prepared for both IPv4 and IPv6:
```sourcepawn
public void OnSocketIncoming(Handle socket, Handle newSocket,
                             const char[] remoteIP, int remotePort, any arg) {
    // remoteIP will be IPv4 (192.0.2.1) or IPv6 ([2001:db8::1])
    PrintToServer("Connection from %s:%d", remoteIP, remotePort);
}
```

### 4. Test Both Protocols

Always test your plugin with both IPv4 and IPv6:
```bash
# Test IPv4
sm plugins load myplugin
sm_connect_test 192.0.2.1

# Test IPv6
sm_connect_test 2001:db8::1

# Test dual-stack
sm_connect_test example.com
```

### 5. Document IPv6 Requirements

If your plugin requires IPv6, document it:
```sourcepawn
/**
 * MyPlugin
 * 
 * Requirements:
 * - SourceMod 1.11+
 * - Socket Extension 2.0+
 * - IPv6 connectivity (for ipv6.example.com)
 */
```

### 6. Graceful Fallback

Implement fallback logic for IPv6 failures:
```sourcepawn
public void OnSocketError(Handle socket, const int errorType,
                          const int errorNum, any arg) {
    if (errorType == NO_HOST && g_bTriedIPv6) {
        // Retry with IPv4
        g_bTriedIPv6 = false;
        Handle newSocket = SocketCreate(SOCKET_TCP, OnSocketError);
        SocketSetOption(newSocket, PreferIPv4, 1);
        SocketConnect(newSocket, OnConnect, OnReceive, OnDisconnect,
                      "example.com", 80);
    }
}
```

## IPv6 Address Notation

### In URLs

IPv6 addresses in URLs must be enclosed in brackets:
```
http://[2001:db8::1]:8080/path
https://[2606:2800:220:1:248:1893:25c8:1946]/
```

### In SourcePawn Code

No brackets needed in socket functions:
```sourcepawn
// Correct
SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "2001:db8::1", 80);

// Incorrect (don't use brackets)
// SocketConnect(socket, OnConnect, OnReceive, OnDisconnect, "[2001:db8::1]", 80);
```

## Performance Considerations

### IPv6 vs IPv4 Performance

- **Latency:** Usually similar, sometimes IPv6 is faster (better routing)
- **Throughput:** No significant difference
- **Connection Time:** May be slightly slower due to dual-stack resolution

### Optimization Tips

1. **Use PreferIPv6** on IPv6-native networks for best performance
2. **Cache DNS results** if making multiple connections to same host
3. **Set PreferIPv4** if you know IPv6 has issues in your environment

## Compatibility

### Extension Version Support

- **v2.0+:** Full IPv6 support
- **v1.x:** IPv4 only

### SourceMod Version

- **1.11+:** Recommended
- **1.10:** May work but untested

### Platform Support

- **Linux:** Full support ✅
- **Windows:** Full support ✅ (untested in v2.0)
- **macOS:** Full support ✅ (untested in v2.0)

## Additional Resources

### Testing IPv6 Connectivity

- [Test IPv6](https://test-ipv6.com/) - Check if your server has IPv6
- [IPv6 Test](https://ipv6-test.com/) - Comprehensive IPv6 test
- [HE IPv6 Certification](https://ipv6.he.net/certification/) - Learn IPv6

### IPv6 Services for Testing

- Google DNS: `2001:4860:4860::8888`, `2001:4860:4860::8844`
- Cloudflare DNS: `2606:4700:4700::1111`, `2606:4700:4700::1001`
- Google (web): `ipv6.google.com`
- Facebook: `ipv6.facebook.com`

### Documentation

- [RFC 4291](https://tools.ietf.org/html/rfc4291) - IPv6 Addressing Architecture
- [RFC 3493](https://tools.ietf.org/html/rfc3493) - Basic Socket Interface Extensions for IPv6
- [Boost.Asio IPv6 Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)

## Support

If you encounter issues with IPv6:

1. Check this guide's troubleshooting section
2. Test with known IPv6 services (Google, Cloudflare)
3. Verify server IPv6 connectivity
4. Report issues on GitHub with:
   - Extension version
   - SourceMod version
   - Server OS and version
   - IPv6 connectivity test results
   - Code example demonstrating the issue

## Changelog

### v2.0.0 (October 5, 2025)
- ✅ Initial IPv6 implementation
- ✅ Dual-stack support (IPv4 and IPv6)
- ✅ IPv6-only mode
- ✅ Protocol preference options
- ✅ IPv4-mapped IPv6 address handling
- ✅ Automatic protocol selection

---

**Version:** 2.0.0  
**Last Updated:** October 5, 2025  
**Status:** Production Ready ✅
