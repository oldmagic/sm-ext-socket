#ifndef INC_SEXT_CALLBACK_H
#define INC_SEXT_CALLBACK_H

#include <string>
#include <boost/asio.hpp>

#include "Define.h"

struct SocketWrapper;

/**
 * @brief Represents a callback event for socket operations
 * 
 * This class encapsulates callback data and manages the execution of
 * SourceMod plugin callbacks for various socket events (connect, disconnect,
 * receive, error, etc.). It handles proper type conversion and cleanup.
 */
class Callback {
public:
	/**
	 * @brief Construct a connect, disconnect or sendqueueempty callback
	 * @param callbackEvent The type of callback event
	 * @param socket Pointer to the socket object
	 */
	Callback(CallbackEvent callbackEvent, const void* socket);

	/**
	 * @brief Construct a receive callback
	 * @param callbackEvent The type of callback event (must be CallbackEvent_Receive)
	 * @param socket Pointer to the socket object
	 * @param data Received data buffer
	 * @param dataLength Length of received data
	 */
	Callback(CallbackEvent callbackEvent, const void* socket, const char* data, size_t dataLength);

	/**
	 * @brief Construct an incoming connection callback
	 * @param callbackEvent The type of callback event (must be CallbackEvent_Incoming)
	 * @param socket Pointer to the listening socket
	 * @param newSocket Pointer to the newly accepted socket
	 * @param remoteEndPoint Remote endpoint information
	 */
	Callback(CallbackEvent callbackEvent, const void* socket, const void* newSocket, const boost::asio::ip::tcp::endpoint& remoteEndPoint);

	/**
	 * @brief Construct an error callback
	 * @param callbackEvent The type of callback event (must be CallbackEvent_Error)
	 * @param socket Pointer to the socket object
	 * @param errorType Type of error that occurred
	 * @param errorNumber System error number
	 */
	Callback(CallbackEvent callbackEvent, const void* socket, SM_ErrorType errorType, int errorNumber);

	~Callback();

	/**
	 * @brief Check if callback can be executed (has valid function pointer)
	 */
	bool IsExecutable();
	
	/**
	 * @brief Check if callback data is valid
	 */
	bool IsValid();

	/**
	 * @brief Execute the callback on the game thread
	 */
	void Execute();

	friend class CallbackHandler;

private:
	template<class SocketType> void ExecuteHelper();

	const CallbackEvent callbackEvent;
	SocketWrapper* socketWrapper;
	const void* additionalData[2];
};

#endif
