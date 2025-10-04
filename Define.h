#ifndef INC_SEXT_DEFINE_H
#define INC_SEXT_DEFINE_H

#include <memory>

/**
 * @brief Socket error types for callback error reporting
 */
enum class SM_ErrorType {
    EMPTY_HOST = 1,    ///< Empty hostname provided
    NO_HOST,           ///< Host resolution failed
    CONNECT_ERROR,     ///< Connection error
    SEND_ERROR,        ///< Send operation failed
    BIND_ERROR,        ///< Bind operation failed
    RECV_ERROR,        ///< Receive operation failed
    LISTEN_ERROR,      ///< Listen operation failed
    TLS_HANDSHAKE_ERROR, ///< TLS handshake failed
    TLS_CERT_ERROR,    ///< TLS certificate verification failed
    TLS_VERSION_ERROR, ///< TLS version not supported
};

/**
 * @brief Socket protocol types
 */
enum class SM_SocketType {
    Tcp = 1,  ///< TCP (connection-oriented)
    Udp,      ///< UDP (connectionless)
    Tls,      ///< TLS/SSL over TCP (secure)
};

/**
 * @brief Socket configuration options
 */
enum class SM_SocketOption {
    // SourceMod level options
    ConcatenateCallbacks = 1,  ///< Concatenate receive callbacks
    ForceFrameLock,            ///< Force frame locking
    CallbacksPerFrame,         ///< Max callbacks per frame
    // Socket level options
    SocketBroadcast,           ///< SO_BROADCAST
    SocketReuseAddr,           ///< SO_REUSEADDR
    SocketKeepAlive,           ///< SO_KEEPALIVE
    SocketLinger,              ///< SO_LINGER
    SocketOOBInline,           ///< SO_OOBINLINE
    SocketSendBuffer,          ///< SO_SNDBUF
    SocketReceiveBuffer,       ///< SO_RCVBUF
    SocketDontRoute,           ///< SO_DONTROUTE
    SocketReceiveLowWatermark, ///< SO_RCVLOWAT
    SocketReceiveTimeout,      ///< SO_RCVTIMEO
    SocketSendLowWatermark,    ///< SO_SNDLOWAT
    SocketSendTimeout,         ///< SO_SNDTIMEO
    // TLS/SSL options
    TLSVerifyPeer,             ///< Verify peer certificate (bool)
    TLSVerifyHost,             ///< Verify hostname matches certificate (bool)
    TLSMinVersion,             ///< Minimum TLS version (TLSv1.2 = 12, TLSv1.3 = 13)
    TLSMaxVersion,             ///< Maximum TLS version (TLSv1.2 = 12, TLSv1.3 = 13)
    TLSCertificateFile,        ///< Path to certificate file
    TLSPrivateKeyFile,         ///< Path to private key file
    TLSCAFile,                 ///< Path to CA certificate file
    // Extension options
    DebugMode                  ///< Enable debug logging
};

/**
 * @brief Helper structure for queued socket options
 */
struct SocketOption {
    constexpr SocketOption(SM_SocketOption opt, int val) noexcept : option(opt), value(val) {}

    SM_SocketOption option;
    int value;
};

/**
 * @brief Types of callback events
 */
enum class CallbackEvent {
    Connect = 0,      ///< Socket connected
    Disconnect,       ///< Socket disconnected
    Incoming,         ///< Incoming connection accepted
    Receive,          ///< Data received
    SendQueueEmpty,   ///< Send queue emptied
    Error,            ///< Error occurred
};

// For backward compatibility with old code (will be removed in later refactoring)
constexpr int SM_ErrorType_EMPTY_HOST = static_cast<int>(SM_ErrorType::EMPTY_HOST);
constexpr int SM_ErrorType_NO_HOST = static_cast<int>(SM_ErrorType::NO_HOST);
constexpr int SM_ErrorType_CONNECT_ERROR = static_cast<int>(SM_ErrorType::CONNECT_ERROR);
constexpr int SM_ErrorType_SEND_ERROR = static_cast<int>(SM_ErrorType::SEND_ERROR);
constexpr int SM_ErrorType_BIND_ERROR = static_cast<int>(SM_ErrorType::BIND_ERROR);
constexpr int SM_ErrorType_RECV_ERROR = static_cast<int>(SM_ErrorType::RECV_ERROR);
constexpr int SM_ErrorType_LISTEN_ERROR = static_cast<int>(SM_ErrorType::LISTEN_ERROR);
constexpr int SM_ErrorType_TLS_HANDSHAKE_ERROR = static_cast<int>(SM_ErrorType::TLS_HANDSHAKE_ERROR);
constexpr int SM_ErrorType_TLS_CERT_ERROR = static_cast<int>(SM_ErrorType::TLS_CERT_ERROR);
constexpr int SM_ErrorType_TLS_VERSION_ERROR = static_cast<int>(SM_ErrorType::TLS_VERSION_ERROR);

constexpr int SM_SocketType_Tcp = static_cast<int>(SM_SocketType::Tcp);
constexpr int SM_SocketType_Udp = static_cast<int>(SM_SocketType::Udp);
constexpr int SM_SocketType_Tls = static_cast<int>(SM_SocketType::Tls);

constexpr int SM_SO_ConcatenateCallbacks = static_cast<int>(SM_SocketOption::ConcatenateCallbacks);
constexpr int SM_SO_ForceFrameLock = static_cast<int>(SM_SocketOption::ForceFrameLock);
constexpr int SM_SO_CallbacksPerFrame = static_cast<int>(SM_SocketOption::CallbacksPerFrame);
constexpr int SM_SO_DebugMode = static_cast<int>(SM_SocketOption::DebugMode);

constexpr int CallbackEvent_Connect = static_cast<int>(CallbackEvent::Connect);
constexpr int CallbackEvent_Disconnect = static_cast<int>(CallbackEvent::Disconnect);
constexpr int CallbackEvent_Incoming = static_cast<int>(CallbackEvent::Incoming);
constexpr int CallbackEvent_Receive = static_cast<int>(CallbackEvent::Receive);
constexpr int CallbackEvent_SendQueueEmpty = static_cast<int>(CallbackEvent::SendQueueEmpty);
constexpr int CallbackEvent_Error = static_cast<int>(CallbackEvent::Error);

#endif // INC_SEXT_DEFINE_H
