#ifndef INC_SEXT_SOCKET_H
#define INC_SEXT_SOCKET_H

#include <stdint.h>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "sdk/smsdk_ext.h"
#include "Define.h"

class SocketHandler;

/**
 * @brief Template class for managing TCP and UDP socket operations
 * 
 * This class handles all socket operations including connecting, sending,
 * receiving, and listening. It uses Boost.Asio for asynchronous operations
 * and thread-safe callback management.
 * 
 * @tparam SocketType The type of socket (tcp or udp from boost::asio::ip)
 */
template <class SocketType>
class Socket {
public:
	Socket(SM_SocketType st, typename SocketType::socket* asioSocket = NULL);
	~Socket();

	bool IsOpen();

	bool Bind(const char* hostname, uint16_t port, bool async = true);
	bool Connect(const char* hostname, uint16_t port, bool async = true);
	bool Disconnect();
	bool Listen();
	bool Send(const std::string& data, bool async = true);
	bool SendTo(const std::string& data, const char* hostname, uint16_t port, bool async = true);
	bool SetOption(SM_SocketOption so, int value, bool lock=true);

	// SourceMod callback functions
	IPluginFunction* connectCallback;
	IPluginFunction* incomingCallback;
	IPluginFunction* receiveCallback;
	IPluginFunction* sendqueueEmptyCallback;
	IPluginFunction* disconnectCallback;
	IPluginFunction* errorCallback;

	int32_t smHandle;
	int32_t smCallbackArg;
	volatile unsigned int sendQueueLength;

private:
	void ReceiveHandler(char* buf, size_t bufferSize, size_t bytes, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);

	void BindPostResolveHandler(typename SocketType::resolver*, typename SocketType::resolver::iterator, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);

	void ConnectPostResolveHandler(typename SocketType::resolver*, typename SocketType::resolver::iterator, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);
	void ConnectPostConnectHandler(typename SocketType::resolver*, typename SocketType::resolver::iterator, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);

	void ListenIncomingHandler(boost::asio::ip::tcp::socket* newAsioSocket, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);

	void SendPostSendHandler(char* buf, size_t bytes, const boost::system::error_code& err, boost::shared_lock<boost::shared_mutex>*);

	void SendToPostResolveHandler(typename SocketType::resolver*, typename SocketType::resolver::iterator, char* buf, size_t bufLen, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);
	void SendToPostSendHandler(typename SocketType::resolver*, typename SocketType::resolver::iterator, char* buf, size_t bufLen, size_t bytesTransferred, const boost::system::error_code&, boost::shared_lock<boost::shared_mutex>*);

	void InitializeSocket();

	SM_SocketType sm_sockettype;
	std::queue<SocketOption*> socketOptionQueue;

	typename SocketType::socket* socket;
	boost::mutex socketMutex;
	typename SocketType::endpoint* localEndpoint;
	boost::mutex* localEndpointMutex;
	boost::asio::ip::tcp::acceptor* tcpAcceptor;
	boost::mutex* tcpAcceptorMutex;

	boost::shared_mutex handlerMutex;
};

#endif