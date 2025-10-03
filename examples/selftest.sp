/**
 * Socket extension selftest
 */

#include <sourcemod>
#include <socket>

public Plugin myinfo = {
	name = "socket extension selftest",
	author = "Player",
	description = "basic functionality testing",
	version = "1.1.0",
	url = "http://www.player.to/"
};

int goalsReached;
int trapsReached;

int test;
public void OnGameFrame() {
	if (test == 0) {
		test++;

		SocketSetOption(INVALID_HANDLE, DebugMode, 1);

		selfTest1();
	}
}

/**
 * TEST #1 - binary data over tcp
 *
 * stuff tested:
 * - create, close
 * - connect
 * - send, receive
 * - listen
 * - sendQueueEmpty
 * - setoption reuseaddr
 * - data containing x00
 *
 * This test is using three sockets:
 * - Listening socket on port 12345
 * - A socket which tries to connect to the listening socket and receives data
 * - the child socket which sends the data
 */
void selfTest1() {
	goalsReached = 0;
	trapsReached = 0;
	CreateTimer(3.0, selfTest1Terminate);
	PrintToServer("* |socket selftest| test #1 running");

	Handle socket = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);

	SocketSetOption(socket, SocketReuseAddr, 1);

	SocketBind(socket, "0.0.0.0", 12345);
	SocketListen(socket, Test1_OnListenSocketIncoming);

	Handle socket2 = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);
	SocketConnect(socket2, OnSocketConnectGoal, Test1_OnReceiveSocketReceive, Test1_OnReceiveSocketDisconnect, "127.0.0.1", 12345);
}

public Action selfTest1Terminate(Handle timer) {
	if (goalsReached == 6 && trapsReached == 0) {
		PrintToServer("* |socket selftest| ** test #1 passed **");
		selfTest2();
	} else {
		PrintToServer("* |socket selftest| ** test #1 failed **");
	}

	return Plugin_Stop;
}

/**
 * TEST #2 - reuse listening address from test #1
 *
 * stuff tested:
 * - create, close
 * - listen
 * - setoption reuseaddr
 *
 * This test is using one socket:
 * - Listening socket on port 12345
 */
void selfTest2() {
	goalsReached = 0;
	trapsReached = 0;
	CreateTimer(4.0, selfTest2Terminate);
	PrintToServer("* |socket selftest| test #2 running");

	Handle socket = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);

	SocketSetOption(socket, SocketReuseAddr, 1);

	SocketBind(socket, "0.0.0.0", 12345);
	SocketListen(socket, OnSocketIncomingTrap);

	CloseHandle(socket);

	goalsReached++;
}

public Action selfTest2Terminate(Handle timer) {
	if (goalsReached == 1 && trapsReached == 0) {
		PrintToServer("* |socket selftest| ** test #2 passed **");
		selfTest3();
	} else {
		PrintToServer("* |socket selftest| ** test #2 failed **");
	}

	return Plugin_Stop;
}

void selfTest3() {
	goalsReached = 0;
	trapsReached = 0;
	CreateTimer(1.0, selfTest3Terminate);
	PrintToServer("* |socket selftest| test #3 running");

	Handle socket[20];
	
	for (int count=1; count <= 20; count++) {
		for (int i=0; i<count; i++) {
			socket[i] = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);
		}

		for (int i=0; i<count; i++) {
			CloseHandle(socket[i]);
		}
	}

	goalsReached++;
}

public Action selfTest3Terminate(Handle timer) {
	if (goalsReached == 1 && trapsReached == 0) {
		PrintToServer("* |socket selftest| ** test #3 passed **");
		selfTest4();
	} else {
		PrintToServer("* |socket selftest| ** test #3 failed **");
	}

	return Plugin_Stop;
}

/**
 * TEST #4 - connect, closehandle
 *
 * stuff tested:
 * - create, close
 * - connect
 * - listen
 */
void selfTest4() {
	goalsReached = 0;
	trapsReached = 0;
	CreateTimer(5.0, selfTest4Terminate);
	PrintToServer("* |socket selftest| test #4 running");

	Handle listenSocket = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);

	SocketSetOption(listenSocket, SocketReuseAddr, 1);

	SocketBind(listenSocket, "0.0.0.0", 12345);
	SocketListen(listenSocket, Test3_OnListenSocketIncoming);

	Handle socket[8];
	
	for (int count=1; count <= 8; count++) {
		for (int i=0; i<count; i++) {
			socket[i] = SocketCreate(SOCKET_TCP, OnSocketErrorTrap);
		}

		for (int i=0; i<count; i++) {
			SocketConnect(socket[i], OnSocketConnectGoal, OnSocketReceiveTrap, OnSocketDisconnectGoal, "127.0.0.1", 12345);
		}
	}

	CloseHandle(listenSocket);

	goalsReached++;
}

public Action selfTest4Terminate(Handle timer) {
	if (goalsReached == 109 && trapsReached == 0) {
		PrintToServer("* |socket selftest| ** test #4 passed **");
		selfTest5();
	} else {
		PrintToServer("* |socket selftest| ** test #4 failed ** (%d goals of 109)", goalsReached);
	}

	return Plugin_Stop;
}

void selfTest5() {
}

// ------------------------------------- TEST 1 callbacks -------------------------------------

public void Test1_OnListenSocketIncoming(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	SocketSetReceiveCallback(newSocket, OnSocketReceiveTrap);
	SocketSetDisconnectCallback(newSocket, OnSocketDisconnectTrap);
	SocketSetErrorCallback(newSocket, OnSocketErrorTrap);

	SocketSend(newSocket, "\x00abc\x00def\x01\x02\x03\x04", 12);
	SocketSetSendqueueEmptyCallback(newSocket, Test1_OnChildSocketSQEmpty);

	PrintToServer("* |socket selftest| goal Test1_OnListenSocketIncoming reached");
	goalsReached++;

	// close listening socket
	CloseHandle(socket);

	PrintToServer("* |socket selftest| goal Test1_OnListenSocketIncoming - CloseHandle reached");
	goalsReached++;
}

public void Test1_OnChildSocketSQEmpty(Handle socket, any arg) {
	CloseHandle(socket);
	PrintToServer("* |socket selftest| goal Test1_OnChildSocketSQEmpty reached");
	goalsReached++;
}

char recvBuffer[128];
int recvBufferPos = 0;

public void Test1_OnReceiveSocketReceive(Handle socket, const char[] receiveData, const int dataSize, any arg) {
	PrintToServer("received %d bytes", dataSize);

	if (recvBufferPos < 512) {
		for (int i=0; i<dataSize && recvBufferPos<sizeof(recvBuffer); i++, recvBufferPos++) {
			recvBuffer[recvBufferPos] = receiveData[i];
		}
	}
}

public void Test1_OnReceiveSocketDisconnect(Handle socket, any arg) {
	PrintToServer("* |socket selftest| goal Test1_OnReceiveSocketDisconnect reached");
	goalsReached++;

	char cmp[] = "\x00abc\x00def\x01\x02\x03\x04";
	int i;
	for (i=0; i<recvBufferPos && i<12; i++) {
		if (recvBuffer[i] != cmp[i]) {
			PrintToServer("comparison failed");
			break;
		}
	}

	CloseHandle(socket);

	if (i == 12) {
		PrintToServer("* |socket selftest| goal Test1_OnReceiveSocketDisconnect - i=12 reached");
		goalsReached++;
	} else {
		PrintToServer("* |socket selftest| trap Test1_OnReceiveSocketDisconnect - i=%d!=12 triggered", i);
		trapsReached++;
	}
}

// ------------------------------------- TEST 3 callbacks -------------------------------------

public void Test3_OnListenSocketIncoming(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	CloseHandle(newSocket);
	goalsReached++;
}

// ------------------------------------- common callbacks -------------------------------------

public void OnSocketConnectGoal(Handle socket, any arg) {
	goalsReached++;
}
public void OnSocketReceiveGoal(Handle socket, const char[] receiveData, const int dataSize, any arg) {
	goalsReached++;
}
public void OnSocketDisconnectGoal(Handle socket, any arg) {
	goalsReached++;
}
public void OnSocketErrorGoal(Handle socket, const int errorType, const int errorNum, any arg) {
	goalsReached++;
}
public void OnSocketIncomingGoal(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	goalsReached++;
}

public void OnSocketConnectTrap(Handle socket, any arg) {
	PrintToServer("* |socket selftest| trap OnSocketConnectTrap triggered");
	trapsReached++;
}
public void OnSocketReceiveTrap(Handle socket, const char[] receiveData, const int dataSize, any arg) {
	PrintToServer("* |socket selftest| trap OnSocketReceiveTrap triggered (%d bytes received)", dataSize);
	trapsReached++;
}
public void OnSocketDisconnectTrap(Handle socket, any arg) {
	PrintToServer("* |socket selftest| trap OnSocketDisconnectTrap triggered");
	trapsReached++;
}
public void OnSocketErrorTrap(Handle socket, const int errorType, const int errorNum, any arg) {
	PrintToServer("* |socket selftest| trap OnSocketErrorTrap triggered (error %d, errno %d)", errorType, errorNum);
	trapsReached++;
}
public void OnSocketIncomingTrap(Handle socket, Handle newSocket, const char[] remoteIP, int remotePort, any arg) {
	PrintToServer("* |socket selftest| trap OnSocketIncomingTrap triggered");
	trapsReached++;
}

// EOF
