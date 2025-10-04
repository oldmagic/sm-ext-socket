/**
 * TLS/SSL Socket Example for SourceMod
 * 
 * This example demonstrates how to use the TLS socket extension
 * to make secure HTTPS connections.
 * 
 * Compilation:
 *   spcomp tls_example.sp
 */

#include <sourcemod>
#include <socket>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo = {
    name = "TLS Socket Example",
    author = "Your Name",
    description = "Example of using TLS sockets for HTTPS requests",
    version = "1.0.0",
    url = "https://github.com/alliedmodders/sm-ext-socket"
};

Handle g_Socket = INVALID_HANDLE;

public void OnPluginStart() {
    RegServerCmd("test_tls", Command_TestTLS, "Test TLS connection");
}

public void OnPluginEnd() {
    if (g_Socket != INVALID_HANDLE) {
        CloseHandle(g_Socket);
        g_Socket = INVALID_HANDLE;
    }
}

/**
 * Command to test TLS connection
 */
public Action Command_TestTLS(int args) {
    PrintToServer("[TLS Example] Testing secure connection...");
    
    // Create a TLS socket
    g_Socket = SocketCreate(SOCKET_TLS, OnSocketError);
    if (g_Socket == INVALID_HANDLE) {
        PrintToServer("[TLS Example] ERROR: Failed to create TLS socket");
        return Plugin_Handled;
    }
    
    // Configure TLS options
    // Enable peer certificate verification
    SocketSetTLSOption(g_Socket, TLSVerifyPeer, 1);
    
    // Enable hostname verification
    SocketSetTLSOption(g_Socket, TLSVerifyHost, 1);
    
    // Set minimum TLS version to 1.2
    SocketSetTLSOption(g_Socket, TLSMinVersion, 12);
    
    // Optional: Set custom CA certificate bundle
    // SocketSetTLSOptionString(g_Socket, TLSCAFile, "configs/ca-bundle.crt");
    
    // Connect to a secure server (HTTPS on port 443)
    if (!SocketConnectTLS(g_Socket, OnSocketConnected, OnSocketReceive, 
                          OnSocketDisconnected, "api.github.com", 443)) {
        PrintToServer("[TLS Example] ERROR: Failed to initiate TLS connection");
        CloseHandle(g_Socket);
        g_Socket = INVALID_HANDLE;
        return Plugin_Handled;
    }
    
    PrintToServer("[TLS Example] Connecting to api.github.com:443...");
    return Plugin_Handled;
}

/**
 * Called when the socket successfully connects and completes TLS handshake
 */
public void OnSocketConnected(Handle socket, any arg) {
    PrintToServer("[TLS Example] ✓ Connected! TLS handshake successful");
    PrintToServer("[TLS Example] Sending HTTPS request...");
    
    // Send an HTTPS request
    char request[512];
    FormatEx(request, sizeof(request),
        "GET /repos/alliedmodders/sourcemod HTTP/1.1\r\n" ...
        "Host: api.github.com\r\n" ...
        "User-Agent: SourceMod-TLS-Example/1.0\r\n" ...
        "Accept: application/json\r\n" ...
        "Connection: close\r\n" ...
        "\r\n");
    
    SocketSendTLS(socket, request);
}

/**
 * Called when encrypted data is received from the server
 */
public void OnSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any arg) {
    PrintToServer("[TLS Example] Received %d bytes (encrypted data automatically decrypted):", dataSize);
    
    // Print first 200 characters of response
    char preview[201];
    int len = (dataSize < 200) ? dataSize : 200;
    FormatEx(preview, sizeof(preview), "%s", receiveData);
    PrintToServer("%s", preview);
    
    if (dataSize < 200) {
        PrintToServer("...");
    }
}

/**
 * Called when the socket disconnects
 */
public void OnSocketDisconnected(Handle socket, any arg) {
    PrintToServer("[TLS Example] Disconnected from server");
    
    CloseHandle(socket);
    g_Socket = INVALID_HANDLE;
}

/**
 * Called when a socket error occurs
 */
public void OnSocketError(Handle socket, const int errorType, const int errorNum, any arg) {
    char errorMsg[256];
    
    switch (errorType) {
        case EMPTY_HOST: {
            FormatEx(errorMsg, sizeof(errorMsg), "Empty hostname provided");
        }
        case NO_HOST: {
            FormatEx(errorMsg, sizeof(errorMsg), "DNS resolution failed");
        }
        case CONNECT_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "Connection failed (error %d)", errorNum);
        }
        case TLS_HANDSHAKE_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "TLS handshake failed (error %d)", errorNum);
        }
        case TLS_CERT_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "Certificate verification failed (error %d)", errorNum);
        }
        case TLS_VERSION_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "TLS version not supported (error %d)", errorNum);
        }
        case SEND_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "Send failed (error %d)", errorNum);
        }
        case RECV_ERROR: {
            FormatEx(errorMsg, sizeof(errorMsg), "Receive failed (error %d)", errorNum);
        }
        default: {
            FormatEx(errorMsg, sizeof(errorMsg), "Unknown error type %d (error %d)", errorType, errorNum);
        }
    }
    
    PrintToServer("[TLS Example] ERROR: %s", errorMsg);
    
    if (socket != INVALID_HANDLE) {
        CloseHandle(socket);
        if (socket == g_Socket) {
            g_Socket = INVALID_HANDLE;
        }
    }
}

/**
 * Example: Testing with self-signed certificate
 * 
 * For testing with self-signed certificates, you may want to disable
 * certificate verification (NOT recommended for production):
 * 
 * SocketSetTLSOption(socket, TLSVerifyPeer, 0);
 * 
 * Or provide a custom CA certificate:
 * 
 * SocketSetTLSOptionString(socket, TLSCAFile, "configs/my-ca.crt");
 */

/**
 * Example: Mutual TLS (client certificate authentication)
 * 
 * For APIs that require client certificates:
 * 
 * SocketSetTLSOptionString(socket, TLSCertificateFile, "configs/client.crt");
 * SocketSetTLSOptionString(socket, TLSPrivateKeyFile, "configs/client.key");
 */
