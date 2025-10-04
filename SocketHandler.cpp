#include "SocketHandler.h"
#include "Socket.h"

using asio::ip::tcp;
using asio::ip::udp;

SocketHandler socketHandler;

// ========================================================================
// SocketWrapper Destructor
// ========================================================================

SocketWrapper::~SocketWrapper() {
    if (socket) {
        switch (socketType) {
            case SM_SocketType::Tcp:
                delete static_cast<Socket<tcp>*>(socket);
                break;
            case SM_SocketType::Udp:
                delete static_cast<Socket<udp>*>(socket);
                break;
        }
        socket = nullptr;
    }
}

// ========================================================================
// SocketHandler Constructor and Destructor
// ========================================================================

SocketHandler::SocketHandler()
    : ioService(std::make_unique<asio::io_service>()),
      ioServiceWork_(std::make_unique<asio::io_service::work>(*ioService)) {
}

SocketHandler::~SocketHandler() {
    Shutdown();
}

// ========================================================================
// Shutdown
// ========================================================================

void SocketHandler::Shutdown() {
    StopProcessing();

    std::lock_guard<std::mutex> lock(socketListMutex_);
    socketList_.clear();
}

// ========================================================================
// GetSocketWrapper
// ========================================================================

SocketWrapper* SocketHandler::GetSocketWrapper(const void* socket) {
    std::lock_guard<std::mutex> lock(socketListMutex_);

    for (auto& sw : socketList_) {
        if (sw->socket == socket) {
            return sw.get();
        }
    }

    return nullptr;
}

// ========================================================================
// CreateSocket
// ========================================================================

template <class SocketType>
Socket<SocketType>* SocketHandler::CreateSocket(SM_SocketType st) {
    auto* socket = new Socket<SocketType>(st);

    std::lock_guard<std::mutex> lock(socketListMutex_);
    socketList_.push_back(std::make_unique<SocketWrapper>(socket, st));

    return socket;
}

// Template instantiations
template Socket<tcp>* SocketHandler::CreateSocket<tcp>(SM_SocketType st);
template Socket<udp>* SocketHandler::CreateSocket<udp>(SM_SocketType st);

// ========================================================================
// DestroySocket
// ========================================================================

void SocketHandler::DestroySocket(SocketWrapper* sw) {
    if (!sw) return;

    std::lock_guard<std::mutex> lock(socketListMutex_);

    for (auto it = socketList_.begin(); it != socketList_.end(); ++it) {
        if (it->get() == sw) {
            socketList_.erase(it);
            return;
        }
    }
}

// ========================================================================
// StartProcessing
// ========================================================================

void SocketHandler::StartProcessing() {
    if (ioServiceProcessingThreadInitialized_) {
        return;
    }

    ioServiceProcessingThread_ = std::make_unique<std::thread>([this]() { RunIoService(); });
    ioServiceProcessingThreadInitialized_ = true;
}

// ========================================================================
// StopProcessing
// ========================================================================

void SocketHandler::StopProcessing() {
    if (!ioServiceProcessingThreadInitialized_) {
        return;
    }

    ioServiceWork_.reset();

    if (ioService) {
        ioService->stop();
    }

    if (ioServiceProcessingThread_ && ioServiceProcessingThread_->joinable()) {
        ioServiceProcessingThread_->join();
    }

    ioServiceProcessingThread_.reset();
    ioServiceProcessingThreadInitialized_ = false;
}

// ========================================================================
// RunIoService
// ========================================================================

void SocketHandler::RunIoService() {
    if (ioService) {
        ioService->run();
    }
}
