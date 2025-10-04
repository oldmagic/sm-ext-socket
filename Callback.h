#ifndef INC_SEXT_CALLBACK_H
#define INC_SEXT_CALLBACK_H

#include <memory>
#include <string>
#include <variant>

#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
#else
#include <asio.hpp>
#endif

#include "Define.h"

struct SocketWrapper;

/**
 * @brief Represents a callback event for socket operations
 *
 * Modern C++17 implementation using:
 * - std::variant for type-safe callback data storage
 * - std::unique_ptr for automatic memory management
 * - std::string instead of raw char pointers
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
     * @param callbackEvent The type of callback event (must be CallbackEvent::Receive)
     * @param socket Pointer to the socket object
     * @param data Received data buffer
     * @param dataLength Length of received data
     */
    Callback(CallbackEvent callbackEvent, const void* socket, const char* data, size_t dataLength);

    /**
     * @brief Construct an incoming connection callback
     * @param callbackEvent The type of callback event (must be CallbackEvent::Incoming)
     * @param socket Pointer to the listening socket
     * @param newSocket Pointer to the newly accepted socket
     * @param remoteEndPoint Remote endpoint information
     */
    Callback(CallbackEvent callbackEvent, const void* socket, const void* newSocket,
             const asio::ip::tcp::endpoint& remoteEndPoint);

    /**
     * @brief Construct an error callback
     * @param callbackEvent The type of callback event (must be CallbackEvent::Error)
     * @param socket Pointer to the socket object
     * @param errorType Type of error that occurred
     * @param errorNumber System error number
     */
    Callback(CallbackEvent callbackEvent, const void* socket, SM_ErrorType errorType, int errorNumber);

    // Destructor
    ~Callback() = default;

    // Delete copy operations
    Callback(const Callback&) = delete;
    Callback& operator=(const Callback&) = delete;

    // Enable move operations
    Callback(Callback&&) noexcept = default;
    Callback& operator=(Callback&&) noexcept = default;

    /**
     * @brief Check if callback can be executed (has valid function pointer)
     * @return true if callback is executable
     */
    [[nodiscard]] bool IsExecutable() const;

    /**
     * @brief Check if callback data is valid
     * @return true if callback data is valid
     */
    [[nodiscard]] bool IsValid() const;

    /**
     * @brief Execute the callback on the game thread
     */
    void Execute();

    friend class CallbackHandler;

private:
    template <class SocketType>
    void ExecuteHelper();

#ifdef ENABLE_TLS
    void ExecuteHelperTLS();
#endif

    // Callback event type
    const CallbackEvent callbackEvent_;

    // Socket wrapper pointer (not owned)
    SocketWrapper* socketWrapper_{nullptr};

    // Type-safe variant for additional callback data
    // Using std::variant instead of void* array for type safety
    struct ReceiveData {
        std::string data;
        size_t dataLength;
    };

    struct IncomingData {
        const void* newSocket;
        asio::ip::tcp::endpoint remoteEndpoint;
    };

    struct ErrorData {
        SM_ErrorType errorType;
        int errorNumber;
    };

    std::variant<std::monostate, ReceiveData, IncomingData, ErrorData> callbackData_;
};

#endif // INC_SEXT_CALLBACK_H
