/**
 * Socket extension sendto selftest
 */

#include <sourcemod>
#include <socket>

public Plugin myinfo = {
	name = "socket extension sendto selftest",
	author = "Player",
	description = "basic functionality testing",
	version = "1.0.0",
	url = "http://www.player.to/"
};
 
public void OnPluginStart() {
	SocketSetOption(INVALID_HANDLE, DebugMode, 1);
	
	int port = 12346;

	Handle socket = SocketCreate(SOCKET_UDP, OnLSocketError);

	SocketBind(socket, "0.0.0.0", port);
	SocketListen(socket, OnLSocketIncoming);

	Handle socket2 = SocketCreate(SOCKET_UDP, OnCSocketError);
	//SocketConnect(socket2, OnCSocketConnect, OnCSocketReceive, OnCSocketDisconnect, "127.0.0.1", port);
}

public void OnLSocketIncoming(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	PrintToServer("%s:%d connected", remoteIP, remotePort);

	SocketSetReceiveCallback(newSocket, OnChildSocketReceive);
	SocketSetDisconnectCallback(newSocket, OnChildSocketDisconnect);
	SocketSetErrorCallback(newSocket, OnChildSocketError);

	SocketSend(newSocket, "\x00abc\x00def\x01\x02\x03\x04", 12);
	SocketSetSendqueueEmptyCallback(newSocket, OnChildSocketSQEmpty);
}

public void OnLSocketError(Handle socket, const int errorType, const int errorNum, any arg) {
	LogError("listen socket error %d (errno %d)", errorType, errorNum);
	CloseHandle(socket);
}

public void OnChildSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any arg) {
	// send (echo) the received data back
	//SocketSend(socket, receiveData);
	// close the connection/socket/handle if it matches quit
	//if (strncmp(receiveData, "quit", 4) == 0) CloseHandle(socket);
}

public void OnChildSocketSQEmpty(Handle socket, any arg) {
	PrintToServer("sq empty");
	CloseHandle(socket);
}

public void OnChildSocketDisconnect(Handle socket, any arg) {
	// remote side disconnected
	PrintToServer("disc");
	CloseHandle(socket);
}

public void OnChildSocketError(Handle socket, const int errorType, const int errorNum, any arg) {
	// a socket error occured

	LogError("child socket error %d (errno %d)", errorType, errorNum);
	CloseHandle(socket);
}

public void OnCSocketConnect(Handle socket, any arg) {
	// send (echo) the received data back
	//SocketSend(socket, receiveData);
	// close the connection/socket/handle if it matches quit
	//if (strncmp(receiveData, "quit", 4) == 0) CloseHandle(socket);
}

char recvBuffer[128];
int recvBufferPos = 0;

public void OnCSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any arg) {
	PrintToServer("received %d bytes", dataSize);

	if (recvBufferPos < 512) {
		for (int i=0; i<dataSize && recvBufferPos<sizeof(recvBuffer); i++, recvBufferPos++) {
			recvBuffer[recvBufferPos] = receiveData[i];
		}
	}
	// send (echo) the received data back
	//SocketSend(socket, receiveData);
	// close the connection/socket/handle if it matches quit
	//if (strncmp(receiveData, "quit", 4) == 0) CloseHandle(socket);
}

public void OnCSocketDisconnect(Handle socket, any arg) {
	char cmp[] = "\x00abc\x00def\x01\x02\x03\x04";
	int i;
	for (i=0; i<recvBufferPos && i<12; i++) {
		if (recvBuffer[i] != cmp[i]) {
			PrintToServer("comparison failed");
			break;
		}
	}

	PrintToServer("comparison finished at pos %d/%d", i, recvBufferPos);

	CloseHandle(socket);
}

public void OnCSocketError(Handle socket, const int errorType, const int errorNum, any arg) {
	// a socket error occured

	LogError("connect socket error %d (errno %d)", errorType, errorNum);
	CloseHandle(socket);
}
