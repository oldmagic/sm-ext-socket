#include "Callback.h"
#include "CallbackHandler.h"
#include "Socket.h"
#include "SocketHandler.h"

#ifdef ENABLE_TLS
#include "SocketTLS.h"
#endif

#include <boost/system/error_code.hpp>

using asio::ip::tcp;
using asio::ip::udp;

extern SocketHandler socketHandler;

// ========================================================================
// Constructors
// ========================================================================

Callback::Callback(CallbackEvent callbackEvent, const void* socket)
    : callbackEvent_(callbackEvent), socketWrapper_(socketHandler.GetSocketWrapper(socket)) {
}

Callback::Callback(CallbackEvent callbackEvent, const void* socket, const char* data, size_t dataLength)
    : callbackEvent_(callbackEvent) {
    socketWrapper_ = socketHandler.GetSocketWrapper(socket);
    callbackData_ = ReceiveData{std::string(data, dataLength), dataLength};
}

Callback::Callback(CallbackEvent callbackEvent, const void* socket, const void* newSocket,
                   const asio::ip::tcp::endpoint& remoteEndPoint)
    : callbackEvent_(callbackEvent), socketWrapper_(socketHandler.GetSocketWrapper(socket)) {
    callbackData_ = IncomingData{newSocket, remoteEndPoint};
}

Callback::Callback(CallbackEvent callbackEvent, const void* socket, SM_ErrorType errorType, int errorNumber)
    : callbackEvent_(callbackEvent), socketWrapper_(socketHandler.GetSocketWrapper(socket)) {
    callbackData_ = ErrorData{errorType, errorNumber};
}

// ========================================================================
// IsExecutable
// ========================================================================

bool Callback::IsExecutable() const {
    if (!socketWrapper_) return false;

    switch (callbackEvent_) {
        case CallbackEvent::Connect:
            return socketWrapper_->socketType == SM_SocketType::Tcp &&
                   static_cast<Socket<tcp>*>(socketWrapper_->socket)->connectCallback != nullptr;
        case CallbackEvent::Disconnect:
            return socketWrapper_->socketType == SM_SocketType::Tcp &&
                   static_cast<Socket<tcp>*>(socketWrapper_->socket)->disconnectCallback != nullptr;
        case CallbackEvent::Incoming:
            return socketWrapper_->socketType == SM_SocketType::Tcp &&
                   static_cast<Socket<tcp>*>(socketWrapper_->socket)->incomingCallback != nullptr;
        case CallbackEvent::Receive:
            return (socketWrapper_->socketType == SM_SocketType::Tcp &&
                    static_cast<Socket<tcp>*>(socketWrapper_->socket)->receiveCallback != nullptr) ||
                   (socketWrapper_->socketType == SM_SocketType::Udp &&
                    static_cast<Socket<udp>*>(socketWrapper_->socket)->receiveCallback != nullptr);
        case CallbackEvent::SendQueueEmpty:
            return (socketWrapper_->socketType == SM_SocketType::Tcp &&
                    static_cast<Socket<tcp>*>(socketWrapper_->socket)->sendqueueEmptyCallback != nullptr) ||
                   (socketWrapper_->socketType == SM_SocketType::Udp &&
                    static_cast<Socket<udp>*>(socketWrapper_->socket)->sendqueueEmptyCallback != nullptr);
        case CallbackEvent::Error:
            return (socketWrapper_->socketType == SM_SocketType::Tcp &&
                    static_cast<Socket<tcp>*>(socketWrapper_->socket)->errorCallback != nullptr) ||
                   (socketWrapper_->socketType == SM_SocketType::Udp &&
                    static_cast<Socket<udp>*>(socketWrapper_->socket)->errorCallback != nullptr);
        default:
            return false;
    }
}

// ========================================================================
// IsValid
// ========================================================================

bool Callback::IsValid() const {
    return socketWrapper_ != nullptr;
}

// ========================================================================
// Execute
// ========================================================================

void Callback::Execute() {
    if (!socketWrapper_) return;

    switch (socketWrapper_->socketType) {
        case SM_SocketType::Tcp:
            ExecuteHelper<tcp>();
            break;
        case SM_SocketType::Udp:
            ExecuteHelper<udp>();
            break;
#ifdef ENABLE_TLS
        case SM_SocketType::Tls:
            ExecuteHelperTLS();
            break;
#endif
    }
}

// ========================================================================
// ExecuteHelper
// ========================================================================

template <class SocketType>
void Callback::ExecuteHelper() {
    auto* socket = static_cast<Socket<SocketType>*>(socketWrapper_->socket);

    switch (callbackEvent_) {
        case CallbackEvent::Connect: {
            if (socket->connectCallback) {
                socket->connectCallback->PushCell(socket->smHandle);
                socket->connectCallback->PushCell(socket->smCallbackArg);
                socket->connectCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Disconnect: {
            if (socket->disconnectCallback) {
                socket->disconnectCallback->PushCell(socket->smHandle);
                socket->disconnectCallback->PushCell(socket->smCallbackArg);
                socket->disconnectCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Incoming: {
            if (socket->incomingCallback) {
                auto* incomingData = std::get_if<IncomingData>(&callbackData_);
                if (!incomingData) break;

                auto* newSocketWrapper = socketHandler.GetSocketWrapper(incomingData->newSocket);
                if (!newSocketWrapper) break;

                auto* newSocket = static_cast<Socket<tcp>*>(newSocketWrapper->socket);

                socket->incomingCallback->PushCell(socket->smHandle);
                socket->incomingCallback->PushCell(newSocket->smHandle);
                socket->incomingCallback->PushString(incomingData->remoteEndpoint.address().to_string().c_str());
                socket->incomingCallback->PushCell(incomingData->remoteEndpoint.port());
                socket->incomingCallback->PushCell(socket->smCallbackArg);
                socket->incomingCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Receive: {
            if (socket->receiveCallback) {
                auto* receiveData = std::get_if<ReceiveData>(&callbackData_);
                if (!receiveData) break;

                socket->receiveCallback->PushCell(socket->smHandle);
                socket->receiveCallback->PushStringEx(
                    const_cast<char*>(receiveData->data.data()),
                    receiveData->data.size(),
                    SM_PARAM_STRING_COPY | SM_PARAM_STRING_BINARY,
                    0);
                socket->receiveCallback->PushCell(receiveData->data.size());
                socket->receiveCallback->PushCell(socket->smCallbackArg);
                socket->receiveCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::SendQueueEmpty: {
            if (socket->sendqueueEmptyCallback) {
                socket->sendqueueEmptyCallback->PushCell(socket->smHandle);
                socket->sendqueueEmptyCallback->PushCell(socket->smCallbackArg);
                socket->sendqueueEmptyCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Error: {
            if (socket->errorCallback) {
                auto* errorData = std::get_if<ErrorData>(&callbackData_);
                if (!errorData) break;

                socket->errorCallback->PushCell(socket->smHandle);
                socket->errorCallback->PushCell(static_cast<int>(errorData->errorType));
                socket->errorCallback->PushCell(errorData->errorNumber);
                socket->errorCallback->PushCell(socket->smCallbackArg);
                socket->errorCallback->Execute(nullptr);
            }
            break;
        }
    }
}

// Template instantiations
template void Callback::ExecuteHelper<tcp>();
template void Callback::ExecuteHelper<udp>();

#ifdef ENABLE_TLS
// ========================================================================
// ExecuteHelperTLS - Special handling for TLS sockets
// ========================================================================

void Callback::ExecuteHelperTLS() {
    auto* socket = static_cast<SocketTLS*>(socketWrapper_->socket);

    switch (callbackEvent_) {
        case CallbackEvent::Connect: {
            if (socket->connectCallback) {
                socket->connectCallback->PushCell(socket->smHandle);
                socket->connectCallback->PushCell(socket->smCallbackArg);
                socket->connectCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Disconnect: {
            if (socket->disconnectCallback) {
                socket->disconnectCallback->PushCell(socket->smHandle);
                socket->disconnectCallback->PushCell(socket->smCallbackArg);
                socket->disconnectCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Receive: {
            if (socket->receiveCallback) {
                auto* receiveData = std::get_if<ReceiveData>(&callbackData_);
                if (!receiveData) break;

                socket->receiveCallback->PushCell(socket->smHandle);
                socket->receiveCallback->PushStringEx(
                    const_cast<char*>(receiveData->data.c_str()),
                    receiveData->dataLength,
                    SM_PARAM_STRING_COPY | SM_PARAM_STRING_BINARY,
                    0);
                socket->receiveCallback->PushCell(receiveData->dataLength);
                socket->receiveCallback->PushCell(socket->smCallbackArg);
                socket->receiveCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::SendQueueEmpty: {
            if (socket->sendqueueEmptyCallback) {
                socket->sendqueueEmptyCallback->PushCell(socket->smHandle);
                socket->sendqueueEmptyCallback->PushCell(socket->smCallbackArg);
                socket->sendqueueEmptyCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Error: {
            if (socket->errorCallback) {
                auto* errorData = std::get_if<ErrorData>(&callbackData_);
                if (!errorData) break;

                socket->errorCallback->PushCell(socket->smHandle);
                socket->errorCallback->PushCell(static_cast<int>(errorData->errorType));
                socket->errorCallback->PushCell(errorData->errorNumber);
                socket->errorCallback->PushCell(socket->smCallbackArg);
                socket->errorCallback->Execute(nullptr);
            }
            break;
        }

        case CallbackEvent::Incoming:
            // TLS doesn't support incoming connections (listening)
            break;
    }
}
#endif // ENABLE_TLS
