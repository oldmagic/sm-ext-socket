#ifndef INC_SEXT_CALLBACKHANDLER_H
#define INC_SEXT_CALLBACKHANDLER_H

#include <deque>
#include <boost/thread.hpp>

class Callback;
struct SocketWrapper;

/**
 * @brief Manages callbacks for asynchronous socket operations
 * 
 * This class maintains a queue of callbacks and ensures they are executed
 * safely on the game thread. It handles thread-safe addition and removal
 * of callbacks, as well as cleanup when sockets are destroyed.
 * 
 * @note No destructor needed - objects are freed by ~SocketHandler -> ~SocketWrapper -> RemoveCallbacks
 */
class CallbackHandler {
public:
	/**
	 * @brief Add a callback to the execution queue
	 * @param callback Pointer to callback object (ownership transferred)
	 */
	void AddCallback(Callback* callback);
	
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
	Callback* FetchFirstCallback();

	std::deque<Callback*> callbackQueue;
	boost::mutex callbackQueueMutex;
};

extern CallbackHandler callbackHandler;

#endif
