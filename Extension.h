#ifndef INC_SEXT_EXTENSION_H
#define INC_SEXT_EXTENSION_H

#include "sdk/smsdk_ext.h"
#include "SocketHandler.h"

/**
 * @brief Main extension class that manages socket operations
 * 
 * This class implements the SourceMod extension interface and handles
 * the lifecycle of socket operations, including handle management.
 */
class Extension : public SDKExtension, public IHandleTypeDispatch {
public:
	virtual bool SDK_OnLoad(char *error, size_t err_max, bool late);
	virtual void SDK_OnUnload(void);
	void OnHandleDestroy(HandleType_t type, void *object);

	/**
	 * @brief Retrieve a socket wrapper by its handle
	 * @param handle The handle to look up
	 * @return Pointer to SocketWrapper or NULL if invalid
	 */
	SocketWrapper* GetSocketWrapperByHandle(Handle_t handle);

	HandleType_t socketHandleType;
};

extern Extension extension;

#endif
