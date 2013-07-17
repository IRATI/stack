package rina.ipcservice.api;

/** 
 * Implements the IPC API for the part of the requesting application
 */

public interface APService {
	
	/**
	 * This primitive is invoked by the IPC process to the IPC Manager
	 * to indicate the success or failure of the request associated with this port-id. 
	 * @param requestedAPinfo
	 * @param port_id -1 if error, portId otherwise
	 * @param result errorCode if result > 0, ok otherwise
	 * @param resultReason null if no error, error description otherwise
	 */
	public void deliverAllocateResponse(int portId, int result, String resultReason);
	
	/**
	 * This primitive is invoked in response to a sumbitStatus to report the current status of 
	 * an allocation-instance
	 * @param port_id
	 * @param result
	 */
	public void deliverStatus(int port_id, boolean result);
	
	/**
	 * Invoked when a Create_Flow primitive is received at the requested IPC process
	 * @param request
	 * @param callback the IPC Service to call back
	 * @return string if there was no error it is null. If the IPC Manager could not find the
	 * destination application or something else bad happens, it will return a string detailing the error 
	 * (then the callback will never be invoked back)
	 */
	public String deliverAllocateRequest(FlowService request, IPCService callback);
	
	/**
	 * Invoked when a Delete_Flow primitive is received at the requested IPC process
	 * @param request
	 */
	public void deliverDeallocate(int portId);
}