/**
 * TLS/HTTPS Client Example
 * 
 * Demonstrates how to make secure HTTPS requests using the new
 * SocketConnectTLS() native in Socket Extension v2.0.
 * 
 * This example connects to the GitHub API over HTTPS and fetches
 * repository information.
 */

#include <sourcemod>
#include <socket>

#pragma newdecls required
#pragma semicolon 1

public Plugin myinfo = {
    name = "Socket TLS Client Example",
    author = "Socket Extension",
    description = "Demonstrates TLS/HTTPS connections",
    version = "2.0.0",
    url = "https://github.com/oldmagic/sm-ext-socket"
};

Handle g_Socket;

public void OnPluginStart() {
    RegAdminCmd("sm_https_test", Command_HttpsTest, ADMFLAG_ROOT, 
                "Test HTTPS connection to GitHub API");
}

public Action Command_HttpsTest(int client, int args) {
    if (g_Socket != null) {
        CloseHandle(g_Socket);
    }
    
    // Create TCP socket
    g_Socket = SocketCreate(SOCKET_TCP, OnSocketError);
    if (g_Socket == null) {
        ReplyToCommand(client, "[TLS] Failed to create socket");
        return Plugin_Handled;
    }
    
    // Connect to GitHub API with TLS
    // Port 443 is the standard HTTPS port
    if (!SocketConnectTLS(g_Socket, OnSocketConnected, OnSocketReceive,
                          OnSocketDisconnected, "api.github.com", 443)) {
        ReplyToCommand(client, "[TLS] Failed to initiate connection");
        CloseHandle(g_Socket);
        g_Socket = null;
        return Plugin_Handled;
    }
    
    ReplyToCommand(client, "[TLS] Connecting to api.github.com:443...");
    return Plugin_Handled;
}

public void OnSocketConnected(Handle socket, any arg) {
    PrintToServer("[TLS] Successfully connected with TLS/SSL!");
    
    // Send HTTPS request
    // Note: HTTP/1.1 requires Host header
    char request[512];
    Format(request, sizeof(request),
           "GET /repos/oldmagic/sm-ext-socket HTTP/1.1\r\n"
           "Host: api.github.com\r\n"
           "User-Agent: SourceMod-Socket/2.0\r\n"
           "Accept: application/json\r\n"
           "Connection: close\r\n\r\n");
    
    if (!SocketSend(socket, request)) {
        PrintToServer("[TLS] Failed to send request");
        return;
    }
    
    PrintToServer("[TLS] Request sent, waiting for response...");
}

public void OnSocketReceive(Handle socket, const char[] data, int size, any arg) {
    // Print received data
    // For production use, you'd want to buffer and parse the HTTP response
    PrintToServer("[TLS] Received %d bytes:", size);
    
    // Split data into lines for readable output
    char[] buffer = new char[size + 1];
    strcopy(buffer, size + 1, data);
    
    // Print first few lines (headers typically come first)
    char lines[10][256];
    int numLines = ExplodeString(buffer, "\n", lines, sizeof(lines), sizeof(lines[]));
    
    for (int i = 0; i < numLines && i < 10; i++) {
        PrintToServer("  %s", lines[i]);
    }
    
    if (numLines > 10) {
        PrintToServer("  ... (%d more lines)", numLines - 10);
    }
}

public void OnSocketDisconnected(Handle socket, any arg) {
    PrintToServer("[TLS] Disconnected from server");
    
    if (socket == g_Socket) {
        CloseHandle(g_Socket);
        g_Socket = null;
    }
}

public void OnSocketError(Handle socket, int errorType, int errorNum, any arg) {
    PrintToServer("[TLS] Socket error %d (errno %d)", errorType, errorNum);
    
    char errorMsg[256];
    if (SocketGetLastErrorString(socket, errorMsg, sizeof(errorMsg))) {
        PrintToServer("[TLS] Error details: %s", errorMsg);
    }
    
    if (socket == g_Socket) {
        CloseHandle(g_Socket);
        g_Socket = null;
    }
}

public void OnPluginEnd() {
    if (g_Socket != null) {
        SocketDisconnect(g_Socket);
        CloseHandle(g_Socket);
    }
}
