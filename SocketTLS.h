#ifndef INC_SEXT_SOCKET_TLS_H
#define INC_SEXT_SOCKET_TLS_H

#ifdef ENABLE_TLS

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
#include <boost/asio/ssl.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#include <asio/ssl.hpp>
#endif

#include "Define.h"
#include "sdk/smsdk_ext.h"

class SocketHandler;

/**
 * @brief TLS/SSL socket wrapper using asio::ssl::stream
 *
 * This class wraps a TCP socket with TLS/SSL encryption using Asio's SSL streams.
 * It provides secure communication with certificate verification and modern TLS support.
 *
 * Features:
 * - TLS 1.2 and TLS 1.3 support
 * - Certificate verification (peer and hostname)
 * - Customizable SSL contexts
 * - Async and sync operations
 * - Thread-safe operations with std::mutex
 *
 * The class uses asio::ssl::stream<asio::ip::tcp::socket> to wrap the underlying
 * TCP socket with SSL/TLS encryption.
 */
class SocketTLS {
public:
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    using tcp = asio::ip::tcp;

    /**
     * @brief Construct a new TLS Socket object
     * @param sslContext Shared pointer to SSL context (required)
     */
    explicit SocketTLS(std::shared_ptr<asio::ssl::context> sslContext);

    // Destructor
    ~SocketTLS();

    // Delete copy operations (sockets should not be copied)
    SocketTLS(const SocketTLS&) = delete;
    SocketTLS& operator=(const SocketTLS&) = delete;

    // Enable move operations
    SocketTLS(SocketTLS&&) noexcept = default;
    SocketTLS& operator=(SocketTLS&&) noexcept = default;

    /**
     * @brief Check if socket is open
     * @return true if socket is open
     */
    [[nodiscard]] bool IsOpen() const;

    /**
     * @brief Connect to remote address with TLS handshake
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
     * @brief Send data through encrypted socket
     * @param data Data to send
     * @param async Perform operation asynchronously
     * @return true on success
     */
    bool Send(std::string_view data, bool async = true);

    /**
     * @brief Set socket option
     * @param so Option to set
     * @param value Option value
     * @param lock Whether to lock during operation
     * @return true on success
     */
    bool SetOption(SM_SocketOption so, int value, bool lock = true);

    /**
     * @brief Set TLS option with string value (for file paths)
     * @param so Option to set
     * @param value String value (file path)
     * @return true on success
     */
    bool SetTLSOption(SM_SocketOption so, std::string_view value);

    /**
     * @brief Verify peer certificate manually
     * @param preverified Pre-verification status from SSL library
     * @param ctx Verification context
     * @return true if certificate is valid
     */
    bool VerifyCertificate(bool preverified, asio::ssl::verify_context& ctx);

    // SourceMod callback functions (raw pointers managed by SourceMod)
    IPluginFunction* connectCallback{nullptr};
    IPluginFunction* receiveCallback{nullptr};
    IPluginFunction* sendqueueEmptyCallback{nullptr};
    IPluginFunction* disconnectCallback{nullptr};
    IPluginFunction* errorCallback{nullptr};

    int32_t smHandle{0};
    int32_t smCallbackArg{0};
    std::atomic<unsigned int> sendQueueLength{0};

private:
    // Async operation handlers
    void ConnectPostResolveHandler(std::unique_ptr<tcp::resolver> resolver,
                                   tcp::resolver::iterator endpointIterator,
                                   const boost::system::error_code& ec,
                                   std::shared_lock<std::shared_mutex> lock);

    void ConnectPostConnectHandler(std::unique_ptr<tcp::resolver> resolver,
                                   tcp::resolver::iterator endpointIterator,
                                   const boost::system::error_code& ec,
                                   std::shared_lock<std::shared_mutex> lock);

    void ConnectPostHandshakeHandler(std::unique_ptr<tcp::resolver> resolver,
                                    const boost::system::error_code& ec,
                                    std::shared_lock<std::shared_mutex> lock);

    void ReceiveHandler(std::unique_ptr<char[]> buf, size_t bufferSize, size_t bytes,
                       const boost::system::error_code& ec,
                       std::shared_lock<std::shared_mutex> lock);

    void SendPostSendHandler(std::unique_ptr<char[]> buf, size_t bytes,
                            const boost::system::error_code& err,
                            std::shared_lock<std::shared_mutex> lock);

    void InitializeSocket();

    // Member variables
    std::shared_ptr<asio::ssl::context> sslContext_;
    std::unique_ptr<ssl_socket> sslSocket_;
    mutable std::mutex socketMutex_;
    mutable std::shared_mutex handlerMutex_;

    // TLS configuration
    bool verifyPeer_{true};
    bool verifyHost_{true};
    int minTLSVersion_{asio::ssl::context::tlsv12};  // Default to TLS 1.2
    int maxTLSVersion_{asio::ssl::context::tlsv13};  // Default to TLS 1.3
    std::string certificateFile_;
    std::string privateKeyFile_;
    std::string caFile_;
    std::string hostname_;  // For hostname verification

    std::queue<std::unique_ptr<SocketOption>> socketOptionQueue_;
};

#endif // ENABLE_TLS
#endif // INC_SEXT_SOCKET_TLS_H
