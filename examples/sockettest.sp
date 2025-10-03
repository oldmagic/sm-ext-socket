// http://forums.alliedmods.net/showpost.php?p=717597&postcount=67

#pragma semicolon 1
#include <sourcemod>
#include <socket>

#define PLUGIN_VERSION "1.0.0"

public Plugin myinfo = 
{
    name = "Socket Test",
    author = "Unknown",
    description = "Test",
    version = "1.0",
    url = ""
}

public void OnPluginStart()
{
	SocketSetOption(INVALID_HANDLE, DebugMode, 1);
    RegAdminCmd("sm_socket_test", CommandTest, ADMFLAG_CONVARS, "test");
}

public Action CommandTest(int client, int args)
{
    // create a new tcp socket
    Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
    // open a file handle for writing the result
    //Handle hFile = OpenFile("dl.htm", "wb");
    // pass the file handle to the callbacks
    //SocketSetArg(socket, hFile);
    // connect the socket
    SocketConnect(socket, OnSocketConnected, OnSocketReceive, OnSocketDisconnected, "www.sourcemod.net", 80);

    return Plugin_Handled;
}

public void OnSocketConnected(Handle socket, any hFile) {
    // socket is connected, send the http request

    char requestStr[100];
    Format(requestStr, sizeof(requestStr), "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", "index.php", "www.sourcemod.net");
    SocketSend(socket, requestStr);
}

public void OnSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any hFile) {
    // receive another chunk and write it to <modfolder>/dl.htm
    // we could strip the http response header here, but for example's sake we'll leave it in

    //WriteFileString(hFile, receiveData, false);
}

public void OnSocketDisconnected(Handle socket, any hFile) {
    // Connection: close advises the webserver to close the connection when the transfer is finished
    // we're done here

    //CloseHandle(hFile);
    CloseHandle(socket);
}

public void OnSocketError(Handle socket, const int errorType, const int errorNum, any hFile) {
    // a socket error occured

    LogError("socket error %d (errno %d)", errorType, errorNum);
    //CloseHandle(hFile);
    CloseHandle(socket);
}

