#ifndef INC_SEXT_SOCKET_H
#define INC_SEXT_SOCKET_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <system_error>

#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#endif

#include "Define.h"
#include "sdk/smsdk_ext.h"

class SocketHandler;

/**
 * @brief Template class for managing TCP and UDP socket operations
 *
 * Modernized socket class using C++17 features:
 * - std::unique_ptr for automatic memory management (RAII)
 * - std::mutex and std::shared_mutex for thread synchronization
 * - std::string_view for efficient string handling
 * - Lambda functions instead of boost::bind
 * - Move semantics support
 *
 * This class handles all socket operations including connecting, sending,
 * receiving, and listening. It uses Asio for asynchronous operations
 * and thread-safe callback management.
 *
 * @tparam SocketType The type of socket (tcp or udp from asio::ip)
 */
template <class SocketType>
class Socket {
public:
    /**
     * @brief Construct a new Socket object
     * @param st Socket type (TCP/UDP)
     * @param asioSocket Optional existing Asio socket (takes ownership)
     */
    explicit Socket(SM_SocketType st, typename SocketType::socket* asioSocket = nullptr);

    // Destructor
    ~Socket();

    // Delete copy operations (sockets should not be copied)
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Enable move operations
    Socket(Socket&&) noexcept = default;
    Socket& operator=(Socket&&) noexcept = default;

    /**
     * @brief Check if socket is open
     * @return true if socket is open
     */
    [[nodiscard]] bool IsOpen() const;

    /**
     * @brief Bind socket to local address
     * @param hostname Hostname or IP address
     * @param port Port number
     * @param async Perform operation asynchronously
     * @return true on success
     */
    bool Bind(std::string_view hostname, uint16_t port, bool async = true);

    /**
     * @brief Connect to remote address
     * @param hostname Remote hostname or IP address
     * @param port Remote port number
     * @param async Perform operation asynchronously
     * @return true on success
     */
    bool Connect(std::string_view hostname, uint16_t port, bool async = true);

    /**
     * @brief Disconnect socket
     * @return true on success
     */
    bool Disconnect();

    /**
     * @brief Start listening for incoming connections (TCP only)
     * @return true on success
     */
    bool Listen();

    /**
     * @brief Send data through socket
     * @param data Data to send
     * @param async Perform operation asynchronously
     * @return true on success
     */
    bool Send(std::string_view data, bool async = true);

    /**
     * @brief Send datagram to specific address (UDP only)
     * @param data Data to send
     * @param hostname Destination hostname or IP
     * @param port Destination port
     * @param async Perform operation asynchronously
     * @return true on success
     */
    bool SendTo(std::string_view data, std::string_view hostname, uint16_t port, bool async = true);

    /**
     * @brief Set socket option
     * @param so Option to set
     * @param value Option value
     * @param lock Whether to lock during operation
     * @return true on success
     */
    bool SetOption(SM_SocketOption so, int value, bool lock = true);

    // SourceMod callback functions (raw pointers managed by SourceMod)
    IPluginFunction* connectCallback{nullptr};
    IPluginFunction* incomingCallback{nullptr};
    IPluginFunction* receiveCallback{nullptr};
    IPluginFunction* sendqueueEmptyCallback{nullptr};
    IPluginFunction* disconnectCallback{nullptr};
    IPluginFunction* errorCallback{nullptr};

    int32_t smHandle{0};
    int32_t smCallbackArg{0};
    std::atomic<unsigned int> sendQueueLength{0};

private:
    // Async operation handlers (using lambdas instead of boost::bind)
    void ReceiveHandler(std::unique_ptr<char[]> buf, size_t bufferSize, size_t bytes,
                        const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> lock);

    void BindPostResolveHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                typename SocketType::resolver::iterator endpointIterator,
                                const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> lock);

    void ConnectPostResolveHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                   typename SocketType::resolver::iterator endpointIterator,
                                   const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> lock);

    void ConnectPostConnectHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                   typename SocketType::resolver::iterator endpointIterator,
                                   const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> lock);

    void ListenIncomingHandler(std::unique_ptr<asio::ip::tcp::socket> newAsioSocket, const boost::system::error_code& ec,
                               std::shared_lock<std::shared_mutex> lock);

    void SendPostSendHandler(std::unique_ptr<char[]> buf, size_t bytes, const boost::system::error_code& err,
                             std::shared_lock<std::shared_mutex> lock);

    void SendToPostResolveHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                  typename SocketType::resolver::iterator endpointIterator,
                                  std::unique_ptr<char[]> buf, size_t bufLen, const boost::system::error_code& ec,
                                  std::shared_lock<std::shared_mutex> lock);

    void SendToPostSendHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                               typename SocketType::resolver::iterator endpointIterator,
                               std::unique_ptr<char[]> buf, size_t bufLen, size_t bytesTransferred,
                               const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> lock);

    void InitializeSocket();
    
    /**
     * @brief Select preferred endpoint from resolver results based on IPv6 preferences
     * @param begin Iterator to first resolved endpoint
     * @param end Iterator past last resolved endpoint
     * @return Iterator to preferred endpoint, or end if none suitable
     */
    typename SocketType::resolver::iterator SelectPreferredEndpoint(
        typename SocketType::resolver::iterator begin,
        typename SocketType::resolver::iterator end);

    // Member variables using modern C++ idioms
    SM_SocketType socketType_;
    std::queue<std::unique_ptr<SocketOption>> socketOptionQueue_;

    std::unique_ptr<typename SocketType::socket> socket_;
    mutable std::mutex socketMutex_;

    std::unique_ptr<typename SocketType::endpoint> localEndpoint_;
    std::unique_ptr<std::mutex> localEndpointMutex_;

    std::unique_ptr<asio::ip::tcp::acceptor> tcpAcceptor_;
    std::unique_ptr<std::mutex> tcpAcceptorMutex_;

    mutable std::shared_mutex handlerMutex_;
    
    // IPv6 configuration
    bool ipv6Only_{false};        ///< IPv6-only mode (no IPv4-mapped addresses)
    bool preferIPv6_{false};      ///< Prefer IPv6 when both available
    bool preferIPv4_{false};      ///< Prefer IPv4 when both available
};

#endif // INC_SEXT_SOCKET_H