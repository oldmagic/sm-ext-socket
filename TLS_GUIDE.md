# TLS/SSL Support Guide

This guide explains how to use TLS/SSL encryption with the Socket Extension for SourceMod.

## Table of Contents

- [Overview](#overview)
- [Requirements](#requirements)
- [Basic Usage](#basic-usage)
- [Configuration Options](#configuration-options)
- [Certificate Verification](#certificate-verification)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [Security Best Practices](#security-best-practices)

## Overview

The Socket Extension provides full TLS/SSL support for secure communications. This allows you to:

- Make HTTPS requests to web APIs
- Connect to secure game servers
- Implement encrypted client-server communications
- Support both TLS 1.2 and TLS 1.3

## Requirements

- OpenSSL library (libssl-dev on Linux)
- Socket Extension built with `-DENABLE_TLS=ON`
- SourceMod 1.10 or later

## Basic Usage

### Creating a TLS Socket

```sourcepawn
// Create a TLS socket
Handle socket = SocketCreate(SOCKET_TLS, OnSocketError);

// Configure TLS options (optional)
SocketSetTLSOption(socket, TLSVerifyPeer, 1);
SocketSetTLSOption(socket, TLSVerifyHost, 1);

// Connect to a secure server
SocketConnectTLS(socket, OnConnected, OnReceive, OnDisconnect, "api.example.com", 443);
```

### Sending Encrypted Data

```sourcepawn
// Data is automatically encrypted
SocketSendTLS(socket, "GET / HTTP/1.1\r\nHost: api.example.com\r\n\r\n");
```

### Receiving Encrypted Data

```sourcepawn
public void OnReceive(Handle socket, const char[] data, const int dataSize, any arg) {
    // Data is automatically decrypted
    PrintToServer("Received: %s", data);
}
```

## Configuration Options

### Certificate Verification

```sourcepawn
// Enable/disable peer certificate verification (default: enabled)
SocketSetTLSOption(socket, TLSVerifyPeer, 1);

// Enable/disable hostname verification (default: enabled)
SocketSetTLSOption(socket, TLSVerifyHost, 1);
```

### TLS Version

```sourcepawn
// Set minimum TLS version (12 = TLS 1.2, 13 = TLS 1.3)
SocketSetTLSOption(socket, TLSMinVersion, 12);

// Set maximum TLS version
SocketSetTLSOption(socket, TLSMaxVersion, 13);
```

### Custom Certificates

```sourcepawn
// Set custom CA certificate bundle
SocketSetTLSOptionString(socket, TLSCAFile, "configs/ca-bundle.crt");

// Set client certificate for mutual TLS
SocketSetTLSOptionString(socket, TLSCertificateFile, "configs/client.crt");
SocketSetTLSOptionString(socket, TLSPrivateKeyFile, "configs/client.key");
```

## Certificate Verification

### Standard Verification

By default, the extension verifies server certificates against the system's CA store:

```sourcepawn
Handle socket = SocketCreate(SOCKET_TLS, OnError);
SocketSetTLSOption(socket, TLSVerifyPeer, 1);  // Default: enabled
SocketSetTLSOption(socket, TLSVerifyHost, 1);  // Default: enabled
```

### Custom CA Bundle

For self-signed certificates or custom CAs:

```sourcepawn
SocketSetTLSOptionString(socket, TLSCAFile, "configs/my-ca-bundle.crt");
```

### Disabling Verification (NOT RECOMMENDED)

For testing only:

```sourcepawn
SocketSetTLSOption(socket, TLSVerifyPeer, 0);  // Disables certificate verification
```

⚠️ **Warning**: Disabling verification makes your connection vulnerable to man-in-the-middle attacks.

## Examples

### Example 1: Simple HTTPS Request

```sourcepawn
public void MakeHTTPSRequest() {
    Handle socket = SocketCreate(SOCKET_TLS, OnError);
    SocketSetTLSOption(socket, TLSMinVersion, 12);  // Require TLS 1.2+
    
    if (!SocketConnectTLS(socket, OnConnected, OnReceive, OnDisconnect, 
                          "api.github.com", 443)) {
        PrintToServer("Failed to connect");
        CloseHandle(socket);
    }
}

public void OnConnected(Handle socket, any arg) {
    char request[512];
    FormatEx(request, sizeof(request),
        "GET /repos/alliedmodders/sourcemod HTTP/1.1\r\n" ...
        "Host: api.github.com\r\n" ...
        "User-Agent: SourceMod\r\n" ...
        "Connection: close\r\n\r\n");
    
    SocketSendTLS(socket, request);
}

public void OnReceive(Handle socket, const char[] data, const int dataSize, any arg) {
    PrintToServer("Response: %s", data);
}
```

### Example 2: Mutual TLS (Client Certificate)

```sourcepawn
public void ConnectWithClientCert() {
    Handle socket = SocketCreate(SOCKET_TLS, OnError);
    
    // Set client certificate
    SocketSetTLSOptionString(socket, TLSCertificateFile, "configs/client.crt");
    SocketSetTLSOptionString(socket, TLSPrivateKeyFile, "configs/client.key");
    
    // Set CA for server verification
    SocketSetTLSOptionString(socket, TLSCAFile, "configs/server-ca.crt");
    
    SocketConnectTLS(socket, OnConnected, OnReceive, OnDisconnect,
                     "secure.example.com", 8443);
}
```

### Example 3: Self-Signed Certificate

```sourcepawn
public void ConnectToSelfSigned() {
    Handle socket = SocketCreate(SOCKET_TLS, OnError);
    
    // Use custom CA certificate
    SocketSetTLSOptionString(socket, TLSCAFile, "configs/self-signed-ca.crt");
    
    SocketConnectTLS(socket, OnConnected, OnReceive, OnDisconnect,
                     "localhost", 8443);
}
```

## Troubleshooting

### TLS Handshake Errors

**Symptom**: Error code `TLS_HANDSHAKE_ERROR`

**Solutions**:
1. Verify the server supports TLS 1.2 or higher
2. Check if the server certificate is valid
3. Ensure system time is correct (certificates are time-sensitive)
4. Try disabling hostname verification for testing:
   ```sourcepawn
   SocketSetTLSOption(socket, TLSVerifyHost, 0);
   ```

### Certificate Verification Errors

**Symptom**: Error code `TLS_CERT_ERROR`

**Solutions**:
1. Update your system's CA certificate bundle
2. Provide a custom CA file:
   ```sourcepawn
   SocketSetTLSOptionString(socket, TLSCAFile, "path/to/ca-bundle.crt");
   ```
3. For self-signed certificates, add the CA to your custom bundle

### TLS Version Errors

**Symptom**: Error code `TLS_VERSION_ERROR`

**Solutions**:
1. Lower the minimum TLS version:
   ```sourcepawn
   SocketSetTLSOption(socket, TLSMinVersion, 12);  // TLS 1.2
   ```
2. Verify the server supports your required TLS version
3. Check if OpenSSL is up to date

### Connection Refused

**Symptom**: Error code `CONNECT_ERROR`

**Solutions**:
1. Verify the port number (HTTPS typically uses 443)
2. Check firewall settings
3. Ensure the hostname is correct

## Security Best Practices

### 1. Always Verify Certificates

```sourcepawn
// GOOD: Verify certificates
SocketSetTLSOption(socket, TLSVerifyPeer, 1);
SocketSetTLSOption(socket, TLSVerifyHost, 1);

// BAD: Don't disable verification in production
SocketSetTLSOption(socket, TLSVerifyPeer, 0);  // ❌ Vulnerable
```

### 2. Use Modern TLS Versions

```sourcepawn
// GOOD: Require TLS 1.2 or higher
SocketSetTLSOption(socket, TLSMinVersion, 12);

// BAD: Don't allow old TLS versions
// Old versions have known vulnerabilities
```

### 3. Validate Input Data

```sourcepawn
public void OnReceive(Handle socket, const char[] data, const int dataSize, any arg) {
    // GOOD: Validate and sanitize received data
    if (dataSize > MAX_EXPECTED_SIZE) {
        PrintToServer("Warning: Received unexpectedly large data");
        return;
    }
    
    // Process data safely...
}
```

### 4. Handle Errors Gracefully

```sourcepawn
public void OnError(Handle socket, const int errorType, const int errorNum, any arg) {
    // GOOD: Log errors and clean up
    LogError("TLS error: type=%d, num=%d", errorType, errorNum);
    
    if (socket != INVALID_HANDLE) {
        CloseHandle(socket);
    }
    
    // Consider retry logic with backoff
}
```

### 5. Protect Sensitive Data

```sourcepawn
// GOOD: Use TLS for sensitive data
Handle socket = SocketCreate(SOCKET_TLS, OnError);
SocketSendTLS(socket, sensitiveData);

// BAD: Don't send sensitive data over plain TCP
Handle plainSocket = SocketCreate(SOCKET_TCP, OnError);
SocketSend(plainSocket, sensitiveData);  // ❌ Unencrypted
```

### 6. Keep Certificates Updated

- Regularly update your CA certificate bundle
- Monitor certificate expiration dates
- Implement certificate rotation for client certificates

### 7. Use Strong Cipher Suites

The extension uses OpenSSL defaults, which include strong cipher suites. To ensure maximum security:

- Keep OpenSSL updated
- The extension automatically disables SSLv2, SSLv3, TLS 1.0, and TLS 1.1
- TLS 1.2 and 1.3 are supported by default

## Performance Considerations

### Connection Pooling

For frequent connections to the same host, consider reusing sockets:

```sourcepawn
// Keep socket alive for multiple requests
SocketSetOption(socket, SocketKeepAlive, 1);
```

### Buffer Sizes

Adjust buffer sizes for your use case:

```sourcepawn
// Increase receive buffer for large responses
SocketSetOption(socket, SocketReceiveBuffer, 65536);
```

### Async vs Sync

TLS operations are async by default, which is recommended for performance:

```sourcepawn
// This is async and non-blocking
SocketConnectTLS(socket, OnConnected, OnReceive, OnDisconnect, host, port);
```

## Additional Resources

- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [TLS 1.3 RFC](https://tools.ietf.org/html/rfc8446)
- [SourceMod Socket Extension](https://github.com/alliedmodders/sm-ext-socket)

## Support

For issues and questions:
- GitHub Issues: https://github.com/alliedmodders/sm-ext-socket/issues
- AlliedModders Forums: https://forums.alliedmods.net/

## License

This extension is licensed under the GNU General Public License v3.0.
