#ifndef INC_SEXT_SOCKETHANDLER_H
#define INC_SEXT_SOCKETHANDLER_H

#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#endif

#ifdef ENABLE_TLS
#ifdef USE_BOOST_ASIO
#include <boost/asio/ssl.hpp>
#else
#include <asio/ssl.hpp>
#endif
#endif

#include "Socket.h"

/**
 * @brief Wrapper structure for type-erased socket storage
 *
 * This structure allows storing TCP and UDP sockets in a common container
 * while maintaining type information for proper cleanup and operations.
 * Uses smart pointers for automatic memory management.
 */
struct SocketWrapper {
    SocketWrapper(void* sock, SM_SocketType sockType) : socket(sock), socketType(sockType) {}
    ~SocketWrapper();

    // Delete copy operations
    SocketWrapper(const SocketWrapper&) = delete;
    SocketWrapper& operator=(const SocketWrapper&) = delete;

    void* socket;
    SM_SocketType socketType;
};

/**
 * @brief Manages all socket instances and the Asio I/O service
 *
 * Modern C++17 implementation using:
 * - std::unique_ptr for automatic resource management
 * - std::thread instead of boost::thread
 * - std::mutex for thread synchronization
 * - RAII patterns for proper cleanup
 *
 * This class is responsible for creating and destroying sockets, managing
 * the I/O service thread, and maintaining a registry of all active sockets.
 * It ensures proper cleanup on extension unload.
 */
class SocketHandler {
public:
    SocketHandler();
    ~SocketHandler();

    // Delete copy and move operations
    SocketHandler(const SocketHandler&) = delete;
    SocketHandler& operator=(const SocketHandler&) = delete;
    SocketHandler(SocketHandler&&) = delete;
    SocketHandler& operator=(SocketHandler&&) = delete;

    /**
     * @brief Shutdown all sockets and stop I/O processing
     */
    void Shutdown();

    /**
     * @brief Get socket wrapper for a given socket pointer
     * @param socket Raw socket pointer
     * @return Pointer to SocketWrapper or nullptr if not found
     */
    [[nodiscard]] SocketWrapper* GetSocketWrapper(const void* socket);

    /**
     * @brief Create a new socket of specified type
     * @tparam SocketType TCP or UDP socket type from Asio
     * @param st SourceMod socket type identifier
     * @return Pointer to newly created socket
     */
    template <class SocketType>
    Socket<SocketType>* CreateSocket(SM_SocketType st);

#ifdef ENABLE_TLS
    /**
     * @brief Create a new TLS socket
     * @return Pointer to newly created TLS socket
     */
    class SocketTLS* CreateTLSSocket();

    /**
     * @brief Get or create the default SSL context
     * @return Shared pointer to SSL context
     */
    std::shared_ptr<asio::ssl::context> GetSSLContext();
#endif

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

    // Public I/O service for socket operations
    std::unique_ptr<asio::io_service> ioService;

private:
    std::deque<std::unique_ptr<SocketWrapper>> socketList_;
    std::mutex socketListMutex_;

    std::unique_ptr<asio::io_service::work> ioServiceWork_;

    std::unique_ptr<std::thread> ioServiceProcessingThread_;
    bool ioServiceProcessingThreadInitialized_{false};

#ifdef ENABLE_TLS
    std::shared_ptr<asio::ssl::context> sslContext_;
    std::mutex sslContextMutex_;
#endif

    void RunIoService();
};

extern SocketHandler socketHandler;

#endif // INC_SEXT_SOCKETHANDLER_H
