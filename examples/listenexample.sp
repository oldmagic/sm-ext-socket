// listening socket example for the socket extension

#include <sourcemod>
#include <socket>

public Plugin myinfo = {
	name = "listen socket example",
	author = "Player",
	description = "This example provides a simple echo server",
	version = "1.0.1",
	url = "http://www.player.to/"
};
 
public void OnPluginStart() {
	// enable socket debugging (only for testing purposes!)
	SocketSetOption(INVALID_HANDLE, DebugMode, 1);


	// create a new tcp socket
	Handle socket = SocketCreate(SOCKET_TCP, OnSocketError);
	// bind the socket to all interfaces, port 50000
	SocketBind(socket, "0.0.0.0", 50000);
	// let the socket listen for incoming connections
	SocketListen(socket, OnSocketIncoming);
}

public void OnSocketIncoming(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	PrintToServer("%s:%d connected", remoteIP, remotePort);

	// setup callbacks required to 'enable' newSocket
	// newSocket won't process data until these callbacks are set
	SocketSetReceiveCallback(newSocket, OnChildSocketReceive);
	SocketSetDisconnectCallback(newSocket, OnChildSocketDisconnected);
	SocketSetErrorCallback(newSocket, OnChildSocketError);

	SocketSend(newSocket, "send quit to quit\n");
}

public void OnSocketError(Handle socket, const int errorType, const int errorNum, any ary) {
	// a socket error occured

	LogError("socket error %d (errno %d)", errorType, errorNum);
	CloseHandle(socket);
}

public void OnChildSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any hFile) {
	// send (echo) the received data back
	SocketSend(socket, receiveData);
	// close the connection/socket/handle if it matches quit
	if (strncmp(receiveData, "quit", 4) == 0) CloseHandle(socket);
}

public void OnChildSocketDisconnected(Handle socket, any hFile) {
	// remote side disconnected

	CloseHandle(socket);
}

public void OnChildSocketError(Handle socket, const int errorType, const int errorNum, any ary) {
	// a socket error occured

	LogError("child socket error %d (errno %d)", errorType, errorNum);
	CloseHandle(socket);
}
