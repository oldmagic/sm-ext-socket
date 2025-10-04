#include "CallbackHandler.h"
#include "Callback.h"

CallbackHandler callbackHandler;

// ========================================================================
// AddCallback
// ========================================================================

void CallbackHandler::AddCallback(std::unique_ptr<Callback> callback) {
    if (!callback) return;

    std::lock_guard<std::mutex> lock(callbackQueueMutex_);
    callbackQueue_.push_back(std::move(callback));
}

// ========================================================================
// RemoveCallbacks
// ========================================================================

void CallbackHandler::RemoveCallbacks(SocketWrapper* sw) {
    if (!sw) return;

    std::lock_guard<std::mutex> lock(callbackQueueMutex_);

    // Remove all callbacks associated with this socket wrapper
    callbackQueue_.erase(
        std::remove_if(callbackQueue_.begin(), callbackQueue_.end(),
                       [sw](const std::unique_ptr<Callback>& callback) {
                           return callback && callback->socketWrapper_ == sw;
                       }),
        callbackQueue_.end());
}

// ========================================================================
// ExecuteQueuedCallbacks
// ========================================================================

void CallbackHandler::ExecuteQueuedCallbacks() {
    while (auto callback = FetchFirstCallback()) {
        if (callback->IsValid() && callback->IsExecutable()) {
            callback->Execute();
        }
    }
}

// ========================================================================
// FetchFirstCallback
// ========================================================================

std::unique_ptr<Callback> CallbackHandler::FetchFirstCallback() {
    std::lock_guard<std::mutex> lock(callbackQueueMutex_);

    if (callbackQueue_.empty()) {
        return nullptr;
    }

    auto callback = std::move(callbackQueue_.front());
    callbackQueue_.pop_front();
    return callback;
}
