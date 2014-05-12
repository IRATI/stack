package rina.events.api;

/**
 * An event
 * @author eduardgrasa
 *
 */
public interface Event {
	
	public static final String CONNECTIVITY_TO_NEIGHBOR_LOST = "Connectivity to Neighbor Lost";
	public static final String EFCP_CONNECTION_CREATED = "EFCP Connection Created";
	public static final String EFCP_CONNECTION_DELETED = "EFCP Connection Deleted";
	public static final String MANAGEMENT_FLOW_ALLOCATED = "Management Flow Allocated";
	public static final String MANAGEMENT_FLOW_DEALLOCATED = "Management Flow Deallocated";
	public static final String N_MINUS_1_FLOW_ALLOCATED = "N minus 1 Flow Allocated";
	public static final String N_MINUS_1_FLOW_ALLOCATION_FAILED = "N minus 1 Flow Allocation Failed";
	public static final String N_MINUS_1_FLOW_DEALLOCATED = "N minus 1 Flow Deallocated";
	public static final String NEIGHBOR_DECLARED_DEAD = "Neighbor declared dead";
	public static final String NEIGHBOR_ADDED = "Neighbor added";

	/**
	 * The id of the event
	 * @return
	 */
	public String getId();
}
