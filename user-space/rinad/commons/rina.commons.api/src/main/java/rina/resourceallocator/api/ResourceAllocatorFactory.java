package rina.resourceallocator.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the ResourceAllocator. Each 
 * instance of a ResourceAllocator maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface ResourceAllocatorFactory {
	
	/**
	 * Creates a new Resource allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Resource Allocator belongs
	 */
	public ResourceAllocator createResourceAllocator(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing Resource allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Flow Allocator belongs
	 */
	public void destroyResourceAllocator(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing Resource allocator
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where Flow Allocator belongs
	 */
	public ResourceAllocator getResourceAllocator(ApplicationProcessNamingInfo namingInfo);
}
