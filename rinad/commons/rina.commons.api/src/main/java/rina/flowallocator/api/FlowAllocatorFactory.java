package rina.flowallocator.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the FlowAllocator. Each 
 * instance of a FlowAllocator maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface FlowAllocatorFactory {
	
	/**
	 * Creates a new Flow allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Flow Allocator belongs
	 */
	public FlowAllocator createFlowAllocator(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing Flow allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Flow Allocator belongs
	 */
	public void destroyFlowAllocator(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing Flow allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where Flow Allocator belongs
	 */
	public FlowAllocator getFlowAllocator(ApplicationProcessNamingInfo namingInfo);
}
