package rina.ipcprocess.impl.flowallocator.policies;

import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import rina.flowallocator.api.Flow;

/**
 * This policy is used to convert an Allocate Request is into a create_flow request.  
 * Its primary task is to translate the request into the proper QoS-class-set, flow set, 
 * and access control capabilities. 
 * @author eduardgrasa
 *
 */
public interface NewFlowRequestPolicy {
	
	/**
	 * Converts an allocate request into a Flow object
	 * @param FlowRequestEvent the allocate request
	 * @param portId the local port id associated to this flow
	 * @return flow the object with all the required data to create a connection that supports this flow
	 * @throws IPCException if the request cannot be satisfied
	 */
	public Flow generateFlowObject(FlowRequestEvent flowRequestEvent, String difName) throws IPCException; 
}
