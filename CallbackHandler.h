#ifndef INC_SEXT_CALLBACKHANDLER_H
#define INC_SEXT_CALLBACKHANDLER_H

#include <deque>
#include <memory>
#include <mutex>

class Callback;
struct SocketWrapper;

/**
 * @brief Manages callbacks for asynchronous socket operations
 *
 * Modern C++17 implementation using:
 * - std::unique_ptr for automatic memory management
 * - std::mutex for thread synchronization
 * - RAII patterns for proper cleanup
 *
 * This class maintains a queue of callbacks and ensures they are executed
 * safely on the game thread. It handles thread-safe addition and removal
 * of callbacks, as well as cleanup when sockets are destroyed.
 *
 * @note Callbacks are automatically freed when sockets are destroyed via
 *       ~SocketHandler -> ~SocketWrapper -> RemoveCallbacks
 */
class CallbackHandler {
public:
    CallbackHandler() = default;
    ~CallbackHandler() = default;

    // Delete copy and move operations
    CallbackHandler(const CallbackHandler&) = delete;
    CallbackHandler& operator=(const CallbackHandler&) = delete;
    CallbackHandler(CallbackHandler&&) = delete;
    CallbackHandler& operator=(CallbackHandler&&) = delete;

    /**
     * @brief Add a callback to the execution queue
     * @param callback Unique pointer to callback object (ownership transferred)
     */
    void AddCallback(std::unique_ptr<Callback> callback);

    /**
     * @brief Remove all callbacks associated with a socket wrapper
     * @param sw Socket wrapper whose callbacks should be removed
     */
    void RemoveCallbacks(SocketWrapper* sw);

    /**
     * @brief Execute all queued callbacks (called from game frame)
     */
    void ExecuteQueuedCallbacks();

private:
    std::unique_ptr<Callback> FetchFirstCallback();

    std::deque<std::unique_ptr<Callback>> callbackQueue_;
    std::mutex callbackQueueMutex_;
};

extern CallbackHandler callbackHandler;

#endif // INC_SEXT_CALLBACKHANDLER_H
