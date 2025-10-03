/**
 * IPv6 Server Example
 * 
 * Demonstrates how to create a TCP server that listens on IPv6 addresses
 * using the new IPv6 features in Socket Extension v2.0.
 * 
 * This example creates a simple echo server that accepts connections
 * on both IPv4 and IPv6 (dual-stack mode).
 */

#include <sourcemod>
#include <socket>

#pragma newdecls required
#pragma semicolon 1

public Plugin myinfo = {
    name = "Socket IPv6 Server Example",
    author = "Socket Extension",
    description = "Demonstrates IPv6 server with dual-stack support",
    version = "2.0.0",
    url = "https://github.com/oldmagic/sm-ext-socket"
};

#define SERVER_PORT 27016

Handle g_ListenSocket;
ArrayList g_ClientSockets;

public void OnPluginStart() {
    g_ClientSockets = new ArrayList();
    
    RegAdminCmd("sm_ipv6_server", Command_StartServer, ADMFLAG_ROOT,
                "Start IPv6 echo server");
    RegAdminCmd("sm_ipv6_stop", Command_StopServer, ADMFLAG_ROOT,
                "Stop IPv6 echo server");
    RegAdminCmd("sm_ipv6_status", Command_ServerStatus, ADMFLAG_ROOT,
                "Show server status");
}

public Action Command_StartServer(int client, int args) {
    if (g_ListenSocket != null) {
        ReplyToCommand(client, "[IPv6] Server is already running");
        return Plugin_Handled;
    }
    
    // Create TCP socket
    g_ListenSocket = SocketCreate(SOCKET_TCP, OnSocketError);
    if (g_ListenSocket == null) {
        ReplyToCommand(client, "[IPv6] Failed to create socket");
        return Plugin_Handled;
    }
    
    // Bind to all IPv6 interfaces (::)
    // This will also accept IPv4 connections in dual-stack mode
    if (!SocketBindIPv6(g_ListenSocket, "::", SERVER_PORT)) {
        ReplyToCommand(client, "[IPv6] Failed to bind socket");
        CloseHandle(g_ListenSocket);
        g_ListenSocket = null;
        return Plugin_Handled;
    }
    
    // Start listening for connections (dual-stack mode)
    if (!SocketListenDualStack(g_ListenSocket, OnSocketIncoming)) {
        ReplyToCommand(client, "[IPv6] Failed to listen on socket");
        CloseHandle(g_ListenSocket);
        g_ListenSocket = null;
        return Plugin_Handled;
    }
    
    ReplyToCommand(client, "[IPv6] Server started on [::]:%d (dual-stack)", SERVER_PORT);
    ReplyToCommand(client, "[IPv6] Accepting both IPv4 and IPv6 connections");
    
    return Plugin_Handled;
}

public Action Command_StopServer(int client, int args) {
    if (g_ListenSocket == null) {
        ReplyToCommand(client, "[IPv6] Server is not running");
        return Plugin_Handled;
    }
    
    StopServer();
    ReplyToCommand(client, "[IPv6] Server stopped");
    
    return Plugin_Handled;
}

public Action Command_ServerStatus(int client, int args) {
    if (g_ListenSocket == null) {
        ReplyToCommand(client, "[IPv6] Server is not running");
    } else {
        ReplyToCommand(client, "[IPv6] Server is running on port %d", SERVER_PORT);
        ReplyToCommand(client, "[IPv6] Active connections: %d", g_ClientSockets.Length);
    }
    
    return Plugin_Handled;
}

void StopServer() {
    // Disconnect all clients
    for (int i = 0; i < g_ClientSockets.Length; i++) {
        Handle socket = view_as<Handle>(g_ClientSockets.Get(i));
        SocketDisconnect(socket);
        CloseHandle(socket);
    }
    g_ClientSockets.Clear();
    
    // Close listening socket
    if (g_ListenSocket != null) {
        SocketDisconnect(g_ListenSocket);
        CloseHandle(g_ListenSocket);
        g_ListenSocket = null;
    }
}

public void OnSocketIncoming(Handle socket, Handle newSocket, 
                             const char[] remoteIP, int remotePort, any arg) {
    // Determine if IPv4 or IPv6
    bool isIPv6 = (StrContains(remoteIP, ":") != -1);
    char ipType[8];
    Format(ipType, sizeof(ipType), isIPv6 ? "IPv6" : "IPv4");
    
    PrintToServer("[IPv6] New %s connection from [%s]:%d", ipType, remoteIP, remotePort);
    
    // Store client socket
    g_ClientSockets.Push(newSocket);
    
    // Set up receive callback for this client
    SocketSetReceiveCallback(newSocket, OnClientReceive);
    SocketSetDisconnectCallback(newSocket, OnClientDisconnect);
    
    // Send welcome message
    char welcome[128];
    Format(welcome, sizeof(welcome), 
           "Welcome to IPv6 Echo Server!\nYour IP: [%s]:%d (%s)\n",
           remoteIP, remotePort, ipType);
    SocketSend(newSocket, welcome);
}

public void OnClientReceive(Handle socket, const char[] data, int size, any arg) {
    // Echo server - send back what we received
    PrintToServer("[IPv6] Echoing %d bytes", size);
    
    // Echo the data back
    SocketSend(socket, data, size);
}

public void OnClientDisconnect(Handle socket, any arg) {
    PrintToServer("[IPv6] Client disconnected");
    
    // Remove from client list
    int index = g_ClientSockets.FindValue(socket);
    if (index != -1) {
        g_ClientSockets.Erase(index);
    }
    
    CloseHandle(socket);
}

public void OnSocketError(Handle socket, int errorType, int errorNum, any arg) {
    PrintToServer("[IPv6] Socket error %d (errno %d)", errorType, errorNum);
    
    char errorMsg[256];
    if (SocketGetLastErrorString(socket, errorMsg, sizeof(errorMsg))) {
        PrintToServer("[IPv6] Error details: %s", errorMsg);
    }
    
    // If it's the listening socket, stop the server
    if (socket == g_ListenSocket) {
        PrintToServer("[IPv6] Listening socket error, stopping server");
        StopServer();
    }
}

public void OnPluginEnd() {
    StopServer();
}

public void OnMapEnd() {
    // Gracefully close all connections on map change
    StopServer();
}
