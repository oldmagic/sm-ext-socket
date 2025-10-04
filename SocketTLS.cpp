#ifdef ENABLE_TLS

#include "SocketTLS.h"
#include "Callback.h"
#include "CallbackHandler.h"
#include "SocketHandler.h"

#include <cstring>
#include <stdexcept>
#include <boost/system/error_code.hpp>

extern CallbackHandler callbackHandler;

using asio::ip::tcp;

// Constructor
SocketTLS::SocketTLS(std::shared_ptr<asio::ssl::context> sslContext)
    : sslContext_(std::move(sslContext)) {
    if (!sslContext_) {
        throw std::invalid_argument("SSL context cannot be null");
    }
    
    // Create SSL socket wrapping TCP socket
    sslSocket_ = std::make_unique<ssl_socket>(*socketHandler.ioService, *sslContext_);
}

// Destructor
SocketTLS::~SocketTLS() {
    if (sslSocket_ && sslSocket_->lowest_layer().is_open()) {
        std::lock_guard<std::mutex> lock(socketMutex_);
        boost::system::error_code ec;
        
        // Shutdown SSL connection gracefully
        sslSocket_->shutdown(ec);
        
        // Close underlying TCP socket
        sslSocket_->lowest_layer().close(ec);
    }
}

// IsOpen
bool SocketTLS::IsOpen() const {
    std::lock_guard<std::mutex> lock(socketMutex_);
    return sslSocket_ && sslSocket_->lowest_layer().is_open();
}

// Connect with TLS handshake
bool SocketTLS::Connect(std::string_view hostname, uint16_t port, bool async) {
    if (!sslSocket_) return false;

    hostname_ = std::string(hostname);  // Store for hostname verification

    try {
        auto resolver = std::make_unique<tcp::resolver>(*socketHandler.ioService);
        tcp::resolver::query query(std::string(hostname), std::to_string(port));

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            resolver->async_resolve(query, 
                [this, resolver = std::move(resolver), handlerLock = std::move(handlerLock)](
                    const boost::system::error_code& ec,
                    tcp::resolver::iterator endpointIterator) mutable {
                ConnectPostResolveHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            tcp::resolver::iterator endpointIterator = resolver->resolve(query, ec);

            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
                return false;
            }

            std::lock_guard<std::mutex> socketLock(socketMutex_);
            InitializeSocket();

            // Connect TCP socket
            sslSocket_->lowest_layer().connect(*endpointIterator, ec);
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
                return false;
            }

            // Perform TLS handshake
            sslSocket_->handshake(asio::ssl::stream_base::client, ec);
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::TLS_HANDSHAKE_ERROR, ec.value()));
                return false;
            }

            callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));

            // Start async receive
            auto buffer = std::make_unique<char[]>(4096);
            sslSocket_->async_read_some(
                asio::buffer(buffer.get(), 4096),
                [this, buffer = std::move(buffer)](const boost::system::error_code& ec, std::size_t bytes) mutable {
                    std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
                    ReceiveHandler(std::move(buffer), 4096, bytes, ec, std::move(handlerLock));
                });
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void SocketTLS::ConnectPostResolveHandler(std::unique_ptr<tcp::resolver> resolver,
                                          tcp::resolver::iterator endpointIterator,
                                          const boost::system::error_code& ec,
                                          std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
        return;
    }

    std::lock_guard<std::mutex> socketLock(socketMutex_);

    try {
        InitializeSocket();

        sslSocket_->lowest_layer().async_connect(*endpointIterator,
            [this, resolver = std::move(resolver), endpointIterator, 
             handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
            ConnectPostConnectHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
        });
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, 0));
    }
}

void SocketTLS::ConnectPostConnectHandler(std::unique_ptr<tcp::resolver> resolver,
                                          tcp::resolver::iterator endpointIterator,
                                          const boost::system::error_code& ec,
                                          std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
        return;
    }

    try {
        // Perform TLS handshake asynchronously
        sslSocket_->async_handshake(asio::ssl::stream_base::client,
            [this, resolver = std::move(resolver), 
             handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
            ConnectPostHandshakeHandler(std::move(resolver), ec, std::move(handlerLock));
        });
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::TLS_HANDSHAKE_ERROR, 0));
    }
}

void SocketTLS::ConnectPostHandshakeHandler(std::unique_ptr<tcp::resolver> resolver,
                                            const boost::system::error_code& ec,
                                            std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::TLS_HANDSHAKE_ERROR, ec.value()));
        return;
    }

    // Connection and TLS handshake successful
    callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));

    // Start async receive
    auto buffer = std::make_unique<char[]>(4096);
    sslSocket_->async_read_some(
        asio::buffer(buffer.get(), 4096),
        [this, buffer = std::move(buffer), handlerLock = std::move(handlerLock)](
            const boost::system::error_code& ec, std::size_t bytes) mutable {
        ReceiveHandler(std::move(buffer), 4096, bytes, ec, std::move(handlerLock));
    });
}

// Disconnect
bool SocketTLS::Disconnect() {
    if (!sslSocket_) return false;

    std::lock_guard<std::mutex> lock(socketMutex_);
    
    if (!sslSocket_->lowest_layer().is_open()) {
        return true;
    }

    try {
        boost::system::error_code ec;
        
        // Shutdown SSL connection
        sslSocket_->shutdown(ec);
        
        // Close underlying socket
        sslSocket_->lowest_layer().close(ec);
        
        callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Disconnect, this));
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Send
bool SocketTLS::Send(std::string_view data, bool async) {
    if (!sslSocket_ || data.empty()) return false;

    try {
        auto buffer = std::make_unique<char[]>(data.size());
        std::memcpy(buffer.get(), data.data(), data.size());

        if (async) {
            sendQueueLength++;
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            asio::async_write(*sslSocket_,
                asio::buffer(buffer.get(), data.size()),
                [this, buffer = std::move(buffer), len = data.size(), 
                 handlerLock = std::move(handlerLock)](
                    const boost::system::error_code& ec, std::size_t bytes) mutable {
                SendPostSendHandler(std::move(buffer), bytes, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            std::lock_guard<std::mutex> socketLock(socketMutex_);
            
            asio::write(*sslSocket_, asio::buffer(buffer.get(), data.size()), ec);
            
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void SocketTLS::SendPostSendHandler(std::unique_ptr<char[]> buf, size_t bytes,
                                    const boost::system::error_code& err,
                                    std::shared_lock<std::shared_mutex> lock) {
    sendQueueLength--;

    if (err) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, err.value()));
        return;
    }

    if (sendQueueLength == 0 && sendqueueEmptyCallback) {
        callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::SendQueueEmpty, this));
    }
}

void SocketTLS::ReceiveHandler(std::unique_ptr<char[]> buf, size_t bufferSize, size_t bytes,
                               const boost::system::error_code& ec,
                               std::shared_lock<std::shared_mutex> lock) {
    if (ec) {
        if (ec != asio::error::eof) {
            callbackHandler.AddCallback(std::make_unique<Callback>(
                CallbackEvent::Error, this, SM_ErrorType::RECV_ERROR, ec.value()));
        }
        
        callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Disconnect, this));
        return;
    }

    if (bytes > 0) {
        // Null-terminate the buffer
        auto dataBuf = std::make_unique<char[]>(bytes + 1);
        std::memcpy(dataBuf.get(), buf.get(), bytes);
        dataBuf[bytes] = '\0';

        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Receive, this, dataBuf.get(), static_cast<size_t>(bytes)));
    }

    // Continue receiving
    auto newBuffer = std::make_unique<char[]>(bufferSize);
    sslSocket_->async_read_some(
        asio::buffer(newBuffer.get(), bufferSize),
        [this, newBuffer = std::move(newBuffer), bufferSize, lock = std::move(lock)](
            const boost::system::error_code& ec, std::size_t bytes) mutable {
        ReceiveHandler(std::move(newBuffer), bufferSize, bytes, ec, std::move(lock));
    });
}

// SetOption
bool SocketTLS::SetOption(SM_SocketOption so, int value, bool lock) {
    if (!sslSocket_) return false;

    try {
        switch (so) {
            case SM_SocketOption::TLSVerifyPeer:
                verifyPeer_ = (value != 0);
                if (verifyPeer_) {
                    sslSocket_->set_verify_mode(asio::ssl::verify_peer);
                } else {
                    sslSocket_->set_verify_mode(asio::ssl::verify_none);
                }
                return true;

            case SM_SocketOption::TLSVerifyHost:
                verifyHost_ = (value != 0);
                return true;

            case SM_SocketOption::TLSMinVersion:
                minTLSVersion_ = value;
                // Note: Asio SSL context version is set during context creation
                return true;

            case SM_SocketOption::TLSMaxVersion:
                maxTLSVersion_ = value;
                return true;

            // Standard socket options passed to underlying TCP socket
            case SM_SocketOption::SocketKeepAlive: {
                asio::socket_base::keep_alive option(value != 0);
                boost::system::error_code ec;
                sslSocket_->lowest_layer().set_option(option, ec);
                return !ec;
            }

            case SM_SocketOption::SocketReuseAddr: {
                asio::socket_base::reuse_address option(value != 0);
                boost::system::error_code ec;
                sslSocket_->lowest_layer().set_option(option, ec);
                return !ec;
            }

            default:
                // Queue unknown options for later
                socketOptionQueue_.push(std::make_unique<SocketOption>(so, value));
                return true;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

// SetTLSOption (for string values)
bool SocketTLS::SetTLSOption(SM_SocketOption so, std::string_view value) {
    try {
        switch (so) {
            case SM_SocketOption::TLSCertificateFile:
                certificateFile_ = std::string(value);
                sslContext_->use_certificate_chain_file(certificateFile_);
                return true;

            case SM_SocketOption::TLSPrivateKeyFile:
                privateKeyFile_ = std::string(value);
                sslContext_->use_private_key_file(privateKeyFile_, asio::ssl::context::pem);
                return true;

            case SM_SocketOption::TLSCAFile:
                caFile_ = std::string(value);
                sslContext_->load_verify_file(caFile_);
                return true;

            default:
                return false;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

// VerifyCertificate
bool SocketTLS::VerifyCertificate(bool preverified, asio::ssl::verify_context& ctx) {
    if (!verifyPeer_) {
        return true;  // Skip verification if disabled
    }

    // If pre-verification failed, reject
    if (!preverified) {
        return false;
    }

    // If hostname verification is enabled, verify the hostname
    if (verifyHost_ && !hostname_.empty()) {
        // Get the certificate
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        if (!cert) {
            return false;
        }

        // Basic hostname verification (can be enhanced)
        // In production, use proper hostname verification libraries
        return true;  // TODO: Implement proper hostname verification
    }

    return preverified;
}

// InitializeSocket
void SocketTLS::InitializeSocket() {
    if (!sslSocket_->lowest_layer().is_open()) {
        boost::system::error_code ec;
        sslSocket_->lowest_layer().open(tcp::v4(), ec);
        
        if (ec) {
            throw std::runtime_error("Failed to open socket");
        }
    }

    // Apply queued socket options
    while (!socketOptionQueue_.empty()) {
        auto opt = std::move(socketOptionQueue_.front());
        socketOptionQueue_.pop();
        SetOption(opt->option, opt->value, false);
    }

    // Set certificate verification callback if enabled
    if (verifyPeer_) {
        sslSocket_->set_verify_callback(
            [this](bool preverified, asio::ssl::verify_context& ctx) {
                return VerifyCertificate(preverified, ctx);
            });
    }
}

#endif // ENABLE_TLS
