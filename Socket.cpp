#include "Socket.h"
#include "Callback.h"
#include "CallbackHandler.h"
#include "SocketHandler.h"

#include <cstring>
#include <stdexcept>
#include <boost/system/error_code.hpp>

extern CallbackHandler callbackHandler;

using asio::ip::tcp;
using asio::ip::udp;

// Forward declarations for specializations
template<> bool Socket<tcp>::Connect(std::string_view, uint16_t, bool);
template<> bool Socket<udp>::Connect(std::string_view, uint16_t, bool);
template<> void Socket<tcp>::ConnectPostConnectHandler(std::unique_ptr<tcp::resolver>, tcp::resolver::iterator, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> void Socket<udp>::ConnectPostConnectHandler(std::unique_ptr<udp::resolver>, udp::resolver::iterator, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> bool Socket<tcp>::Listen();
template<> bool Socket<udp>::Listen();
template<> void Socket<tcp>::ListenIncomingHandler(std::unique_ptr<asio::ip::tcp::socket>, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> void Socket<udp>::ListenIncomingHandler(std::unique_ptr<asio::ip::tcp::socket>, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> void Socket<tcp>::ReceiveHandler(std::unique_ptr<char[]>, size_t, size_t, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> void Socket<udp>::ReceiveHandler(std::unique_ptr<char[]>, size_t, size_t, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> bool Socket<tcp>::Send(std::string_view, bool);
template<> bool Socket<udp>::Send(std::string_view, bool);
template<> bool Socket<tcp>::SendTo(std::string_view, std::string_view, uint16_t, bool);
template<> bool Socket<udp>::SendTo(std::string_view, std::string_view, uint16_t, bool);
template<> void Socket<tcp>::SendToPostResolveHandler(std::unique_ptr<tcp::resolver>, tcp::resolver::iterator, std::unique_ptr<char[]>, size_t, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);
template<> void Socket<udp>::SendToPostResolveHandler(std::unique_ptr<udp::resolver>, udp::resolver::iterator, std::unique_ptr<char[]>, size_t, const boost::system::error_code&, std::shared_lock<std::shared_mutex>);

// Constructor
template <class SocketType>
Socket<SocketType>::Socket(SM_SocketType st, typename SocketType::socket* asioSocket)
    : socketType_(st),
      socket_(asioSocket ? std::unique_ptr<typename SocketType::socket>(asioSocket)
                         : std::make_unique<typename SocketType::socket>(*socketHandler.ioService)) {
}

// Destructor
template <class SocketType>
Socket<SocketType>::~Socket() {
    if (socket_ && socket_->is_open()) {
        std::lock_guard<std::mutex> lock(socketMutex_);
        boost::system::error_code ec;
        socket_->close(ec);
    }

    if (tcpAcceptor_ && tcpAcceptor_->is_open()) {
        if (tcpAcceptorMutex_) {
            std::lock_guard<std::mutex> lock(*tcpAcceptorMutex_);
            boost::system::error_code ec;
            tcpAcceptor_->close(ec);
        }
    }
}

// IsOpen
template <class SocketType>
bool Socket<SocketType>::IsOpen() const {
    std::lock_guard<std::mutex> lock(socketMutex_);
    return socket_ && socket_->is_open();
}

// Bind
template <class SocketType>
bool Socket<SocketType>::Bind(std::string_view hostname, uint16_t port, bool async) {
    if (!socket_) return false;

    try {
        auto resolver = std::make_unique<typename SocketType::resolver>(*socketHandler.ioService);
        typename SocketType::resolver::query query(std::string(hostname), std::to_string(port));

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            resolver->async_resolve(query, [this, resolver = std::move(resolver), handlerLock = std::move(handlerLock)](
                                               const boost::system::error_code& ec,
                                               typename SocketType::resolver::iterator endpointIterator) mutable {
                BindPostResolveHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            typename SocketType::resolver::iterator endpointIterator = resolver->resolve(query, ec);

            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, ec.value()));
                return false;
            }

            std::lock_guard<std::mutex> socketLock(socketMutex_);
            
            if (!localEndpoint_) {
                localEndpoint_ = std::make_unique<typename SocketType::endpoint>(*endpointIterator);
                localEndpointMutex_ = std::make_unique<std::mutex>();
            } else {
                std::lock_guard<std::mutex> epLock(*localEndpointMutex_);
                *localEndpoint_ = *endpointIterator;
            }

            if (!socket_->is_open()) {
                socket_->open(SocketType::v4(), ec);
                if (ec) {
                    callbackHandler.AddCallback(std::make_unique<Callback>(
                        CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, ec.value()));
                    return false;
                }
            }

            socket_->bind(*localEndpoint_, ec);
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, ec.value()));
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

template <class SocketType>
void Socket<SocketType>::BindPostResolveHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                                 typename SocketType::resolver::iterator endpointIterator,
                                                 const boost::system::error_code& ec,
                                                 std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, ec.value()));
        return;
    }

    std::lock_guard<std::mutex> socketLock(socketMutex_);

    try {
        if (!localEndpoint_) {
            localEndpoint_ = std::make_unique<typename SocketType::endpoint>(*endpointIterator);
            localEndpointMutex_ = std::make_unique<std::mutex>();
        } else {
            std::lock_guard<std::mutex> epLock(*localEndpointMutex_);
            *localEndpoint_ = *endpointIterator;
        }

        boost::system::error_code bindEc;
        if (!socket_->is_open()) {
            socket_->open(SocketType::v4(), bindEc);
            if (bindEc) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, bindEc.value()));
                return;
            }
        }

        socket_->bind(*localEndpoint_, bindEc);
        if (bindEc) {
            callbackHandler.AddCallback(std::make_unique<Callback>(
                CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, bindEc.value()));
        }
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::BIND_ERROR, 0));
    }
}

// Connect - TCP implementation
template <>
bool Socket<tcp>::Connect(std::string_view hostname, uint16_t port, bool async) {
    if (!socket_) return false;

    try {
        auto resolver = std::make_unique<tcp::resolver>(*socketHandler.ioService);
        // Use protocol-agnostic query to resolve both IPv4 and IPv6
        tcp::resolver::query query(std::string(hostname), std::to_string(port),
                                   tcp::resolver::query::flags(tcp::resolver::query::v4_mapped | 
                                                               tcp::resolver::query::all_matching));

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            resolver->async_resolve(query, [this, resolver = std::move(resolver), handlerLock = std::move(handlerLock)](
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
            
            // Select preferred endpoint based on IPv6 settings
            tcp::resolver::iterator selectedEndpoint = SelectPreferredEndpoint(endpointIterator, tcp::resolver::iterator());
            if (selectedEndpoint == tcp::resolver::iterator()) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::NO_HOST, 0));
                return false;
            }

            std::lock_guard<std::mutex> socketLock(socketMutex_);
            InitializeSocket();

            socket_->connect(*selectedEndpoint, ec);
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
                return false;
            }

            callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));

            // Start async receive for TCP
            auto buffer = std::make_unique<char[]>(4096);
            socket_->async_read_some(
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

// Connect - UDP implementation (binds endpoint)
template <>
bool Socket<udp>::Connect(std::string_view hostname, uint16_t port, bool async) {
    if (!socket_) return false;

    try {
        auto resolver = std::make_unique<udp::resolver>(*socketHandler.ioService);
        udp::resolver::query query(std::string(hostname), std::to_string(port));

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            resolver->async_resolve(query, [this, resolver = std::move(resolver), handlerLock = std::move(handlerLock)](
                                               const boost::system::error_code& ec,
                                               udp::resolver::iterator endpointIterator) mutable {
                ConnectPostResolveHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            udp::resolver::iterator endpointIterator = resolver->resolve(query, ec);

            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
                return false;
            }

            std::lock_guard<std::mutex> socketLock(socketMutex_);
            InitializeSocket();

            socket_->connect(*endpointIterator, ec);
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
                return false;
            }

            callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

template <class SocketType>
void Socket<SocketType>::ConnectPostResolveHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                                    typename SocketType::resolver::iterator endpointIterator,
                                                    const boost::system::error_code& ec,
                                                    std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
        return;
    }
    
    // Select preferred endpoint based on IPv6 settings
    typename SocketType::resolver::iterator selectedEndpoint = SelectPreferredEndpoint(endpointIterator, typename SocketType::resolver::iterator());
    if (selectedEndpoint == typename SocketType::resolver::iterator()) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::NO_HOST, 0));
        return;
    }

    std::lock_guard<std::mutex> socketLock(socketMutex_);

    try {
        InitializeSocket();

        socket_->async_connect(*selectedEndpoint,
                              [this, resolver = std::move(resolver), endpointIterator = selectedEndpoint, 
                               handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
            ConnectPostConnectHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
        });
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, 0));
    }
}

// TCP version - starts async receive
template <>
void Socket<tcp>::ConnectPostConnectHandler(std::unique_ptr<tcp::resolver> resolver,
                                            tcp::resolver::iterator endpointIterator,
                                            const boost::system::error_code& ec,
                                            std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        if (++endpointIterator != tcp::resolver::iterator()) {
            std::lock_guard<std::mutex> socketLock(socketMutex_);
            socket_->async_connect(*endpointIterator,
                                  [this, resolver = std::move(resolver), endpointIterator,
                                   handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
                ConnectPostConnectHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
            });
        } else {
            callbackHandler.AddCallback(std::make_unique<Callback>(
                CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
        }
        return;
    }

    callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));

    auto buffer = std::make_unique<char[]>(4096);
    socket_->async_read_some(
        asio::buffer(buffer.get(), 4096),
        [this, buffer = std::move(buffer)](const boost::system::error_code& ec, std::size_t bytes) mutable {
            std::shared_lock<std::shared_mutex> lock(handlerMutex_);
            ReceiveHandler(std::move(buffer), 4096, bytes, ec, std::move(lock));
        });
}

// UDP version - no async receive after connect
template <>
void Socket<udp>::ConnectPostConnectHandler(std::unique_ptr<udp::resolver> resolver,
                                            udp::resolver::iterator endpointIterator,
                                            const boost::system::error_code& ec,
                                            std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        if (++endpointIterator != udp::resolver::iterator()) {
            std::lock_guard<std::mutex> socketLock(socketMutex_);
            socket_->async_connect(*endpointIterator,
                                  [this, resolver = std::move(resolver), endpointIterator,
                                   handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
                ConnectPostConnectHandler(std::move(resolver), endpointIterator, ec, std::move(handlerLock));
            });
        } else {
            callbackHandler.AddCallback(std::make_unique<Callback>(
                CallbackEvent::Error, this, SM_ErrorType::CONNECT_ERROR, ec.value()));
        }
        return;
    }

    callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Connect, this));
}

// Disconnect
template <class SocketType>
bool Socket<SocketType>::Disconnect() {
    if (!socket_) return false;

    std::lock_guard<std::mutex> lock(socketMutex_);

    try {
        if (socket_->is_open()) {
            boost::system::error_code ec;
            socket_->close(ec);
            
            callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Disconnect, this));
            return true;
        }
    } catch (const std::exception& e) {
    }

    return false;
}

// Listen - TCP specialization
template <>
bool Socket<tcp>::Listen() {
    try {
        std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);

        if (!localEndpoint_) {
            throw std::logic_error("local endpoint not initialized, call bind() first");
        }

        if (!tcpAcceptor_) {
            tcpAcceptorMutex_ = std::make_unique<std::mutex>();
            std::lock_guard<std::mutex> tcpAcceptorLock(*tcpAcceptorMutex_);
            std::lock_guard<std::mutex> localEndpointLock(*localEndpointMutex_);

            tcpAcceptor_ = std::make_unique<tcp::acceptor>(*socketHandler.ioService, *localEndpoint_);

            while (!socketOptionQueue_.empty()) {
                auto& opt = socketOptionQueue_.front();
                SetOption(opt->option, opt->value, false);
                socketOptionQueue_.pop();
            }
        }

        auto nextAsioSocket = std::make_unique<tcp::socket>(*socketHandler.ioService);
        auto* nextAsioSocketPtr = nextAsioSocket.get();
        
        std::lock_guard<std::mutex> tcpAcceptorLock(*tcpAcceptorMutex_);
        
        tcpAcceptor_->async_accept(*nextAsioSocketPtr,
                                   [this, nextAsioSocket = std::move(nextAsioSocket),
                                    handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
            ListenIncomingHandler(std::move(nextAsioSocket), ec, std::move(handlerLock));
        });

        return true;
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::LISTEN_ERROR, 0));
        return false;
    }
}

// UDP version - should not be called
template <>
bool Socket<udp>::Listen() {
    return false;  // Not supported for UDP
}

// ListenIncomingHandler - TCP specialization
template <>
void Socket<tcp>::ListenIncomingHandler(std::unique_ptr<asio::ip::tcp::socket> newAsioSocket,
                                        const boost::system::error_code& ec,
                                        std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::LISTEN_ERROR, ec.value()));
        return;
    }

    try {
        auto* newSocket = socketHandler.CreateSocket<tcp>(SM_SocketType::Tcp);
        
        {
            std::lock_guard<std::mutex> lock(newSocket->socketMutex_);
            newSocket->socket_ = std::move(newAsioSocket);
        }

        boost::system::error_code epEc;
        auto remoteEndpoint = newSocket->socket_->remote_endpoint(epEc);

        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Incoming, this, newSocket, remoteEndpoint));

        auto buffer = std::make_unique<char[]>(4096);
        newSocket->socket_->async_read_some(
            asio::buffer(buffer.get(), 4096),
            [newSocket, buffer = std::move(buffer)](const boost::system::error_code& ec, std::size_t bytes) mutable {
                std::shared_lock<std::shared_mutex> lock(newSocket->handlerMutex_);
                newSocket->ReceiveHandler(std::move(buffer), 4096, bytes, ec, std::move(lock));
            });

        auto nextAsioSocket = std::make_unique<tcp::socket>(*socketHandler.ioService);
        auto* nextAsioSocketPtr = nextAsioSocket.get();
        
        std::lock_guard<std::mutex> tcpAcceptorLock(*tcpAcceptorMutex_);
        
        tcpAcceptor_->async_accept(*nextAsioSocketPtr,
                                   [this, nextAsioSocket = std::move(nextAsioSocket),
                                    handlerLock = std::move(handlerLock)](const boost::system::error_code& ec) mutable {
            ListenIncomingHandler(std::move(nextAsioSocket), ec, std::move(handlerLock));
        });
    } catch (const std::exception& e) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::LISTEN_ERROR, 0));
    }
}

// UDP version - should not be called  
template <>
void Socket<udp>::ListenIncomingHandler(std::unique_ptr<asio::ip::tcp::socket> newAsioSocket,
                                        const boost::system::error_code& ec,
                                        std::shared_lock<std::shared_mutex> handlerLock) {
    // Not supported for UDP
}

// ReceiveHandler - TCP version
template <>
void Socket<tcp>::ReceiveHandler(std::unique_ptr<char[]> buf, size_t bufferSize, size_t bytes,
                                  const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        // Check for disconnection errors
        bool isDisconnect = (ec.value() == asio::error::eof || 
                            ec.value() == asio::error::connection_reset);
        
        if (isDisconnect) {
            callbackHandler.AddCallback(std::make_unique<Callback>(CallbackEvent::Disconnect, this));
        } else {
            callbackHandler.AddCallback(std::make_unique<Callback>(
                CallbackEvent::Error, this, SM_ErrorType::RECV_ERROR, ec.value()));
        }
        return;
    }

    if (bytes > 0) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Receive, this, buf.get(), bytes));
    }

    auto newBuffer = std::make_unique<char[]>(bufferSize);
    socket_->async_read_some(
        asio::buffer(newBuffer.get(), bufferSize),
        [this, newBuffer = std::move(newBuffer), bufferSize,
         handlerLock = std::move(handlerLock)](const boost::system::error_code& ec, std::size_t bytes) mutable {
            ReceiveHandler(std::move(newBuffer), bufferSize, bytes, ec, std::move(handlerLock));
        });
}

// ReceiveHandler - UDP version (not used in typical flow, UDP is connectionless)
template <>
void Socket<udp>::ReceiveHandler(std::unique_ptr<char[]> buf, size_t bufferSize, size_t bytes,
                                  const boost::system::error_code& ec, std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::RECV_ERROR, ec.value()));
        return;
    }

    if (bytes > 0) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Receive, this, buf.get(), bytes));
    }
}

// Send - TCP version
template <>
bool Socket<tcp>::Send(std::string_view data, bool async) {
    if (!socket_ || data.empty()) return false;

    try {
        auto buffer = std::make_unique<char[]>(data.size());
        std::memcpy(buffer.get(), data.data(), data.size());

        sendQueueLength++;

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            asio::async_write(*socket_, asio::buffer(buffer.get(), data.size()),
                            [this, buffer = std::move(buffer), len = data.size(),
                             handlerLock = std::move(handlerLock)](const boost::system::error_code& ec, std::size_t bytes) mutable {
                SendPostSendHandler(std::move(buffer), bytes, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            std::lock_guard<std::mutex> lock(socketMutex_);
            asio::write(*socket_, asio::buffer(buffer.get(), data.size()), ec);
            
            sendQueueLength--;
            
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
                return false;
            }

            if (sendQueueLength == 0 && sendqueueEmptyCallback) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::SendQueueEmpty, this));
            }
        }

        return true;
    } catch (const std::exception& e) {
        sendQueueLength--;
        return false;
    }
}

// Send - UDP version (use Send for connected UDP, SendTo for unconnected)
template <>
bool Socket<udp>::Send(std::string_view data, bool async) {
    if (!socket_ || data.empty()) return false;

    try {
        auto buffer = std::make_unique<char[]>(data.size());
        std::memcpy(buffer.get(), data.data(), data.size());

        sendQueueLength++;

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            socket_->async_send(asio::buffer(buffer.get(), data.size()),
                               [this, buffer = std::move(buffer), len = data.size(),
                                handlerLock = std::move(handlerLock)](const boost::system::error_code& ec, std::size_t bytes) mutable {
                SendPostSendHandler(std::move(buffer), bytes, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            std::lock_guard<std::mutex> lock(socketMutex_);
            socket_->send(asio::buffer(buffer.get(), data.size()), 0, ec);
            
            sendQueueLength--;
            
            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
                return false;
            }

            if (sendQueueLength == 0 && sendqueueEmptyCallback) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::SendQueueEmpty, this));
            }
        }

        return true;
    } catch (const std::exception& e) {
        sendQueueLength--;
        return false;
    }
}

template <class SocketType>
void Socket<SocketType>::SendPostSendHandler(std::unique_ptr<char[]> buf, size_t bytes,
                                             const boost::system::error_code& ec,
                                             std::shared_lock<std::shared_mutex> handlerLock) {
    sendQueueLength--;

    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
        return;
    }

    if (sendQueueLength == 0 && sendqueueEmptyCallback) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::SendQueueEmpty, this));
    }
}

// SendTo - UDP specialization
template <>
bool Socket<udp>::SendTo(std::string_view data, std::string_view hostname, uint16_t port, bool async) {
    if (!socket_ || data.empty()) return false;

    try {
        auto resolver = std::make_unique<udp::resolver>(*socketHandler.ioService);
        udp::resolver::query query(std::string(hostname), std::to_string(port));

        auto buffer = std::make_unique<char[]>(data.size());
        std::memcpy(buffer.get(), data.data(), data.size());

        sendQueueLength++;

        if (async) {
            std::shared_lock<std::shared_mutex> handlerLock(handlerMutex_);
            
            resolver->async_resolve(query,
                                   [this, resolver = std::move(resolver), buffer = std::move(buffer),
                                    len = data.size(), handlerLock = std::move(handlerLock)](
                                       const boost::system::error_code& ec,
                                       udp::resolver::iterator endpointIterator) mutable {
                SendToPostResolveHandler(std::move(resolver), endpointIterator,
                                        std::move(buffer), len, ec, std::move(handlerLock));
            });
        } else {
            boost::system::error_code ec;
            udp::resolver::iterator endpointIterator = resolver->resolve(query, ec);

            if (ec) {
                sendQueueLength--;
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
                return false;
            }

            std::lock_guard<std::mutex> lock(socketMutex_);
            InitializeSocket();

            socket_->send_to(asio::buffer(buffer.get(), data.size()), *endpointIterator, 0, ec);
            
            sendQueueLength--;

            if (ec) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
                return false;
            }

            if (sendQueueLength == 0 && sendqueueEmptyCallback) {
                callbackHandler.AddCallback(std::make_unique<Callback>(
                    CallbackEvent::SendQueueEmpty, this));
            }
        }

        return true;
    } catch (const std::exception& e) {
        sendQueueLength--;
        return false;
    }
}

// SendTo - TCP version (not supported)
template <>
bool Socket<tcp>::SendTo(std::string_view data, std::string_view hostname, uint16_t port, bool async) {
    return false;  // Not supported for TCP, use Connect + Send instead
}

// SendToPostResolveHandler - TCP stub (not supported)
template <>
void Socket<tcp>::SendToPostResolveHandler(std::unique_ptr<tcp::resolver> resolver,
                                           tcp::resolver::iterator endpointIterator,
                                           std::unique_ptr<char[]> buf, size_t bufLen,
                                           const boost::system::error_code& ec,
                                           std::shared_lock<std::shared_mutex> handlerLock) {
    // Not supported for TCP
    sendQueueLength--;
}

// SendToPostResolveHandler - UDP specialization only
template <>
void Socket<udp>::SendToPostResolveHandler(std::unique_ptr<udp::resolver> resolver,
                                           udp::resolver::iterator endpointIterator,
                                           std::unique_ptr<char[]> buf, size_t bufLen,
                                           const boost::system::error_code& ec,
                                           std::shared_lock<std::shared_mutex> handlerLock) {
    if (ec) {
        sendQueueLength--;
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::NO_HOST, ec.value()));
        return;
    }

    std::lock_guard<std::mutex> lock(socketMutex_);

    try {
        InitializeSocket();

        socket_->async_send_to(
            asio::buffer(buf.get(), bufLen), *endpointIterator,
            [this, resolver = std::move(resolver), endpointIterator, buf = std::move(buf),
             bufLen, handlerLock = std::move(handlerLock)](const boost::system::error_code& ec, std::size_t bytes) mutable {
                SendToPostSendHandler(std::move(resolver), endpointIterator,
                                     std::move(buf), bufLen, bytes, ec, std::move(handlerLock));
            });
    } catch (const std::exception& e) {
        sendQueueLength--;
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, 0));
    }
}

template <class SocketType>
void Socket<SocketType>::SendToPostSendHandler(std::unique_ptr<typename SocketType::resolver> resolver,
                                               typename SocketType::resolver::iterator endpointIterator,
                                               std::unique_ptr<char[]> buf, size_t bufLen, size_t bytesTransferred,
                                               const boost::system::error_code& ec,
                                               std::shared_lock<std::shared_mutex> handlerLock) {
    sendQueueLength--;

    if (ec) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::Error, this, SM_ErrorType::SEND_ERROR, ec.value()));
        return;
    }

    if (sendQueueLength == 0 && sendqueueEmptyCallback) {
        callbackHandler.AddCallback(std::make_unique<Callback>(
            CallbackEvent::SendQueueEmpty, this));
    }
}

// SetOption
template <class SocketType>
bool Socket<SocketType>::SetOption(SM_SocketOption so, int value, bool lock) {
    try {
        // Handle IPv6 options that don't require an open socket
        switch (so) {
            case SM_SocketOption::IPv6Only:
                ipv6Only_ = (value != 0);
                return true;
            case SM_SocketOption::PreferIPv6:
                preferIPv6_ = (value != 0);
                if (preferIPv6_) preferIPv4_ = false;  // Mutually exclusive
                return true;
            case SM_SocketOption::PreferIPv4:
                preferIPv4_ = (value != 0);
                if (preferIPv4_) preferIPv6_ = false;  // Mutually exclusive
                return true;
            default:
                break;  // Continue to socket options
        }
        
        std::unique_ptr<std::lock_guard<std::mutex>> socketLock;
        if (lock) {
            socketLock = std::make_unique<std::lock_guard<std::mutex>>(socketMutex_);
        }

        if (socket_ && socket_->is_open()) {
            boost::system::error_code ec;
            
            switch (so) {
                case SM_SocketOption::SocketBroadcast:
                    socket_->set_option(asio::socket_base::broadcast(value != 0), ec);
                    break;
                case SM_SocketOption::SocketReuseAddr:
                    socket_->set_option(asio::socket_base::reuse_address(value != 0), ec);
                    break;
                case SM_SocketOption::SocketKeepAlive:
                    socket_->set_option(asio::socket_base::keep_alive(value != 0), ec);
                    break;
                case SM_SocketOption::SocketLinger:
                    socket_->set_option(asio::socket_base::linger(value > 0, value), ec);
                    break;
                case SM_SocketOption::SocketSendBuffer:
                    socket_->set_option(asio::socket_base::send_buffer_size(value), ec);
                    break;
                case SM_SocketOption::SocketReceiveBuffer:
                    socket_->set_option(asio::socket_base::receive_buffer_size(value), ec);
                    break;
                case SM_SocketOption::SocketDontRoute:
                    socket_->set_option(asio::socket_base::do_not_route(value != 0), ec);
                    break;
                default:
                    return true;
            }

            return !ec;
        } else if (tcpAcceptor_ && tcpAcceptor_->is_open()) {
            std::unique_ptr<std::lock_guard<std::mutex>> acceptorLock;
            if (lock) {
                acceptorLock = std::make_unique<std::lock_guard<std::mutex>>(*tcpAcceptorMutex_);
            }

            boost::system::error_code ec;
            
            switch (so) {
                case SM_SocketOption::SocketReuseAddr:
                    tcpAcceptor_->set_option(asio::socket_base::reuse_address(value != 0), ec);
                    break;
                default:
                    return true;
            }

            return !ec;
        } else {
            socketOptionQueue_.push(std::make_unique<SocketOption>(so, value));
            return true;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

// InitializeSocket - supports IPv4 and IPv6
template <class SocketType>
void Socket<SocketType>::InitializeSocket() {
    if (socket_ && !socket_->is_open()) {
        boost::system::error_code ec;
        // Choose protocol based on IPv6 preferences
        if (preferIPv6_ || ipv6Only_) {
            socket_->open(SocketType::v6(), ec);
        } else {
            socket_->open(SocketType::v4(), ec);
        }
    }
}

// SelectPreferredEndpoint - chooses endpoint based on IPv6 preferences
template <class SocketType>
typename SocketType::resolver::iterator Socket<SocketType>::SelectPreferredEndpoint(
    typename SocketType::resolver::iterator begin,
    typename SocketType::resolver::iterator end) {
    
    if (begin == end) {
        return end;  // No endpoints available
    }
    
    // If no preferences set, return first endpoint
    if (!ipv6Only_ && !preferIPv6_ && !preferIPv4_) {
        return begin;
    }
    
    typename SocketType::resolver::iterator ipv6Endpoint = end;
    typename SocketType::resolver::iterator ipv4Endpoint = end;
    
    // Scan through endpoints to find IPv4 and IPv6 options
    for (auto it = begin; it != end; ++it) {
        if (it->endpoint().address().is_v6()) {
            if (ipv6Endpoint == end) {
                ipv6Endpoint = it;
            }
        } else if (it->endpoint().address().is_v4()) {
            if (ipv4Endpoint == end) {
                ipv4Endpoint = it;
            }
        }
        
        // Early exit if we found both types
        if (ipv6Endpoint != end && ipv4Endpoint != end) {
            break;
        }
    }
    
    // Apply preferences
    if (ipv6Only_) {
        return (ipv6Endpoint != end) ? ipv6Endpoint : end;
    }
    
    if (preferIPv6_ && ipv6Endpoint != end) {
        return ipv6Endpoint;
    }
    
    if (preferIPv4_ && ipv4Endpoint != end) {
        return ipv4Endpoint;
    }
    
    // Default: return first available
    return (ipv6Endpoint != end) ? ipv6Endpoint : 
           (ipv4Endpoint != end) ? ipv4Endpoint : begin;
}

// Explicit template instantiations
template typename tcp::resolver::iterator Socket<tcp>::SelectPreferredEndpoint(tcp::resolver::iterator, tcp::resolver::iterator);
template typename udp::resolver::iterator Socket<udp>::SelectPreferredEndpoint(udp::resolver::iterator, udp::resolver::iterator);

// Template Instantiations
template class Socket<tcp>;
template class Socket<udp>;
