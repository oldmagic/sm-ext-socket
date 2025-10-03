#ifndef INC_SEXT_DEFINE_H
#define INC_SEXT_DEFINE_H

/**
 * @brief Socket error types for callback error reporting
 */
enum SM_ErrorType {
	SM_ErrorType_EMPTY_HOST = 1,    ///< Empty hostname provided
	SM_ErrorType_NO_HOST,            ///< Host resolution failed
	SM_ErrorType_CONNECT_ERROR,      ///< Connection error
	SM_ErrorType_SEND_ERROR,         ///< Send operation failed
	SM_ErrorType_BIND_ERROR,         ///< Bind operation failed
	SM_ErrorType_RECV_ERROR,         ///< Receive operation failed
	SM_ErrorType_LISTEN_ERROR,       ///< Listen operation failed
};

/**
 * @brief Socket protocol types
 */
enum SM_SocketType {
	SM_SocketType_Tcp = 1,  ///< TCP (connection-oriented)
	SM_SocketType_Udp,      ///< UDP (connectionless)
};

/**
 * @brief Socket configuration options
 */
enum SM_SocketOption {
	// SourceMod level options
	SM_SO_ConcatenateCallbacks = 1,  ///< Concatenate receive callbacks
	SM_SO_ForceFrameLock,             ///< Force frame locking
	SM_SO_CallbacksPerFrame,          ///< Max callbacks per frame
	// Socket level options
	SM_SO_SocketBroadcast,            ///< SO_BROADCAST
	SM_SO_SocketReuseAddr,            ///< SO_REUSEADDR
	SM_SO_SocketKeepAlive,            ///< SO_KEEPALIVE
	SM_SO_SocketLinger,               ///< SO_LINGER
	SM_SO_SocketOOBInline,            ///< SO_OOBINLINE
	SM_SO_SocketSendBuffer,           ///< SO_SNDBUF
	SM_SO_SocketReceiveBuffer,        ///< SO_RCVBUF
	SM_SO_SocketDontRoute,            ///< SO_DONTROUTE
	SM_SO_SocketReceiveLowWatermark,  ///< SO_RCVLOWAT
	SM_SO_SocketReceiveTimeout,       ///< SO_RCVTIMEO
	SM_SO_SocketSendLowWatermark,     ///< SO_SNDLOWAT
	SM_SO_SocketSendTimeout,          ///< SO_SNDTIMEO
	// Extension options
	SM_SO_DebugMode                   ///< Enable debug logging
};

/**
 * @brief Helper structure for queued socket options
 */
struct SocketOption {
	SocketOption(SM_SocketOption so, int value) : option(so), value(value) {}
	SM_SocketOption option;
	int value;
};

/**
 * @brief Types of callback events
 */
enum CallbackEvent {
	CallbackEvent_Connect = 0,     ///< Socket connected
	CallbackEvent_Disconnect,      ///< Socket disconnected
	CallbackEvent_Incoming,        ///< Incoming connection accepted
	CallbackEvent_Receive,         ///< Data received
	CallbackEvent_SendQueueEmpty,  ///< Send queue emptied
	CallbackEvent_Error,           ///< Error occurred
};

#endif
