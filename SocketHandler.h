#ifndef INC_SEXT_SOCKETHANDLER_H
#define INC_SEXT_SOCKETHANDLER_H

#include <deque>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "Socket.h"

/**
 * @brief Wrapper structure for type-erased socket storage
 * 
 * This structure allows storing TCP and UDP sockets in a common container
 * while maintaining type information for proper cleanup and operations.
 */
struct SocketWrapper {
	SocketWrapper(void* socket, SM_SocketType socketType) : socket(socket), socketType(socketType) {}
	~SocketWrapper();

	void* socket;
	SM_SocketType socketType;
};

/**
 * @brief Manages all socket instances and the Boost.Asio I/O service
 * 
 * This class is responsible for creating and destroying sockets, managing
 * the I/O service thread, and maintaining a registry of all active sockets.
 * It ensures proper cleanup on extension unload.
 */
class SocketHandler {
public:
	SocketHandler();
	~SocketHandler();

	/**
	 * @brief Shutdown all sockets and stop I/O processing
	 */
	void Shutdown();

	/**
	 * @brief Get socket wrapper for a given socket pointer
	 * @param socket Raw socket pointer
	 * @return Pointer to SocketWrapper or NULL if not found
	 */
	SocketWrapper* GetSocketWrapper(const void* socket);

	/**
	 * @brief Create a new socket of specified type
	 * @tparam SocketType TCP or UDP socket type from Boost.Asio
	 * @param st SourceMod socket type identifier
	 * @return Pointer to newly created socket
	 */
	template <class SocketType> Socket<SocketType>* CreateSocket(SM_SocketType st);
	
	/**
	 * @brief Destroy a socket and clean up its resources
	 * @param sw Socket wrapper to destroy
	 */
	void DestroySocket(SocketWrapper* sw);

	/**
	 * @brief Start the I/O service processing thread
	 */
	void StartProcessing();
	
	/**
	 * @brief Stop the I/O service processing thread
	 */
	void StopProcessing();

	boost::asio::io_service* ioService;

private:
	std::deque<SocketWrapper*> socketList;
	boost::mutex socketListMutex;

	boost::asio::io_service::work* ioServiceWork;

	boost::thread* ioServiceProcessingThread;
	bool ioServiceProcessingThreadInitialized;

	void RunIoService();
};

extern SocketHandler socketHandler;

#endif
