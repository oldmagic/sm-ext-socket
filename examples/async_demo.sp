/**
 * Async Socket Demo
 * 
 * Demonstrates advanced asynchronous socket operations including:
 * - Multiple concurrent connections
 * - Non-blocking send/receive
 * - Connection pooling
 * - Error handling and retry logic
 * 
 * This example connects to multiple servers simultaneously and
 * demonstrates how to manage multiple sockets efficiently.
 */

#include <sourcemod>
#include <socket>

#pragma newdecls required
#pragma semicolon 1

public Plugin myinfo = {
    name = "Socket Async Demo",
    author = "Socket Extension",
    description = "Demonstrates async socket operations",
    version = "2.0.0",
    url = "https://github.com/oldmagic/sm-ext-socket"
};

// Connection pool
enum struct Connection {
    Handle socket;
    char hostname[128];
    int port;
    bool isConnected;
    int retries;
    float lastAttempt;
}

ArrayList g_Connections;
int g_MaxRetries = 3;
float g_RetryDelay = 5.0;

public void OnPluginStart() {
    g_Connections = new ArrayList(sizeof(Connection));
    
    RegAdminCmd("sm_async_test", Command_AsyncTest, ADMFLAG_ROOT,
                "Test async connections to multiple servers");
    RegAdminCmd("sm_async_status", Command_AsyncStatus, ADMFLAG_ROOT,
                "Show status of all connections");
    RegAdminCmd("sm_async_clear", Command_AsyncClear, ADMFLAG_ROOT,
                "Clear all connections");
}

public Action Command_AsyncTest(int client, int args) {
    // Add multiple test connections
    AddConnection("api.github.com", 443, true);  // HTTPS
    AddConnection("www.google.com", 80, false);  // HTTP
    AddConnection("example.com", 80, false);     // HTTP
    
    ReplyToCommand(client, "[Async] Started %d async connections", g_Connections.Length);
    ReplyToCommand(client, "[Async] Use sm_async_status to check progress");
    
    return Plugin_Handled;
}

public Action Command_AsyncStatus(int client, int args) {
    if (g_Connections.Length == 0) {
        ReplyToCommand(client, "[Async] No active connections");
        return Plugin_Handled;
    }
    
    ReplyToCommand(client, "[Async] Connection Status:");
    ReplyToCommand(client, "----------------------------------------");
    
    Connection conn;
    for (int i = 0; i < g_Connections.Length; i++) {
        g_Connections.GetArray(i, conn);
        
        char status[32];
        if (conn.isConnected) {
            Format(status, sizeof(status), "CONNECTED");
        } else if (conn.socket != null) {
            Format(status, sizeof(status), "CONNECTING");
        } else {
            Format(status, sizeof(status), "DISCONNECTED (retries: %d)", conn.retries);
        }
        
        ReplyToCommand(client, "[%d] %s:%d - %s", 
                      i, conn.hostname, conn.port, status);
    }
    
    return Plugin_Handled;
}

public Action Command_AsyncClear(int client, int args) {
    ClearAllConnections();
    ReplyToCommand(client, "[Async] Cleared all connections");
    return Plugin_Handled;
}

void AddConnection(const char[] hostname, int port, bool useTLS) {
    Connection conn;
    strcopy(conn.hostname, sizeof(conn.hostname), hostname);
    conn.port = port;
    conn.isConnected = false;
    conn.retries = 0;
    conn.lastAttempt = 0.0;
    
    // Create socket
    conn.socket = SocketCreate(SOCKET_TCP, OnSocketError);
    if (conn.socket == null) {
        PrintToServer("[Async] Failed to create socket for %s:%d", hostname, port);
        return;
    }
    
    int index = g_Connections.PushArray(conn);
    SocketSetArg(conn.socket, index);  // Store index as callback arg
    
    // Start async connection
    bool success;
    if (useTLS) {
        success = SocketConnectTLS(conn.socket, OnSocketConnected, OnSocketReceive,
                                   OnSocketDisconnected, hostname, port);
    } else {
        success = SocketConnect(conn.socket, OnSocketConnected, OnSocketReceive,
                               OnSocketDisconnected, hostname, port);
    }
    
    if (!success) {
        PrintToServer("[Async] Failed to initiate connection to %s:%d", hostname, port);
        CloseHandle(conn.socket);
        conn.socket = null;
        g_Connections.SetArray(index, conn);
    } else {
        PrintToServer("[Async] Connecting to %s:%d...", hostname, port);
    }
}

public void OnSocketConnected(Handle socket, int index) {
    Connection conn;
    g_Connections.GetArray(index, conn);
    
    conn.isConnected = true;
    conn.retries = 0;
    g_Connections.SetArray(index, conn);
    
    PrintToServer("[Async] Connected to %s:%d", conn.hostname, conn.port);
    
    // Send a simple HTTP GET request
    char request[512];
    Format(request, sizeof(request),
           "GET / HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: SourceMod-Socket/2.0\r\n"
           "Connection: close\r\n\r\n",
           conn.hostname);
    
    SocketSend(socket, request);
}

public void OnSocketReceive(Handle socket, const char[] data, int size, int index) {
    Connection conn;
    g_Connections.GetArray(index, conn);
    
    PrintToServer("[Async] Received %d bytes from %s:%d", size, conn.hostname, conn.port);
    
    // Parse first line of HTTP response
    char firstLine[256];
    int pos = StrContains(data, "\r\n");
    if (pos != -1 && pos < sizeof(firstLine)) {
        strcopy(firstLine, pos + 1, data);
        PrintToServer("[Async]   Response: %s", firstLine);
    }
}

public void OnSocketDisconnected(Handle socket, int index) {
    Connection conn;
    g_Connections.GetArray(index, conn);
    
    PrintToServer("[Async] Disconnected from %s:%d", conn.hostname, conn.port);
    
    conn.isConnected = false;
    CloseHandle(conn.socket);
    conn.socket = null;
    
    // Schedule retry if under retry limit
    if (conn.retries < g_MaxRetries) {
        conn.lastAttempt = GetGameTime();
        conn.retries++;
        PrintToServer("[Async] Will retry connection to %s:%d (attempt %d/%d)", 
                     conn.hostname, conn.port, conn.retries + 1, g_MaxRetries + 1);
        CreateTimer(g_RetryDelay, Timer_RetryConnection, index);
    } else {
        PrintToServer("[Async] Max retries reached for %s:%d", 
                     conn.hostname, conn.port);
    }
    
    g_Connections.SetArray(index, conn);
}

public Action Timer_RetryConnection(Handle timer, int index) {
    if (index >= g_Connections.Length) {
        return Plugin_Stop;
    }
    
    Connection conn;
    g_Connections.GetArray(index, conn);
    
    if (conn.socket != null) {
        // Already reconnected
        return Plugin_Stop;
    }
    
    PrintToServer("[Async] Retrying connection to %s:%d...", conn.hostname, conn.port);
    
    // Create new socket and connect
    conn.socket = SocketCreate(SOCKET_TCP, OnSocketError);
    if (conn.socket != null) {
        SocketSetArg(conn.socket, index);
        
        // Determine if we should use TLS (port 443 = HTTPS)
        bool useTLS = (conn.port == 443);
        
        if (useTLS) {
            SocketConnectTLS(conn.socket, OnSocketConnected, OnSocketReceive,
                           OnSocketDisconnected, conn.hostname, conn.port);
        } else {
            SocketConnect(conn.socket, OnSocketConnected, OnSocketReceive,
                        OnSocketDisconnected, conn.hostname, conn.port);
        }
        
        g_Connections.SetArray(index, conn);
    }
    
    return Plugin_Stop;
}

public void OnSocketError(Handle socket, int errorType, int errorNum, int index) {
    Connection conn;
    if (index >= 0 && index < g_Connections.Length) {
        g_Connections.GetArray(index, conn);
        PrintToServer("[Async] Error on %s:%d - type %d, errno %d", 
                     conn.hostname, conn.port, errorType, errorNum);
    } else {
        PrintToServer("[Async] Socket error %d (errno %d)", errorType, errorNum);
    }
    
    char errorMsg[256];
    if (SocketGetLastErrorString(socket, errorMsg, sizeof(errorMsg))) {
        PrintToServer("[Async]   Details: %s", errorMsg);
    }
}

void ClearAllConnections() {
    Connection conn;
    for (int i = 0; i < g_Connections.Length; i++) {
        g_Connections.GetArray(i, conn);
        if (conn.socket != null) {
            SocketDisconnect(conn.socket);
            CloseHandle(conn.socket);
        }
    }
    g_Connections.Clear();
}

public void OnPluginEnd() {
    ClearAllConnections();
    delete g_Connections;
}
