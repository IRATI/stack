package rinad;

public interface RoutingAlgorithm {
	
	/**
	 * Compute the path to other nodes and updates the PDU forwarding Table
	 * @return
	 */
	public int computePath();

}
