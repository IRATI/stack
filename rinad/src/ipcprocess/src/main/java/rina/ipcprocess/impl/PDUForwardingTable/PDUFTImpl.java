package rina.ipcprocess.impl.PDUForwardingTable;

import rina.events.api.Event;
import rina.ipcprocess.impl.IPCProcess;

public class PDUFTImpl /*implements <TODO>*/{

	/**
	 * The IPC process
	 */
	private IPCProcess ipcProcess = null;
	/**
	 * The RIB Daemon
	 */
	private RIBDaemon ribDaemon = null;
	
	/**
	 * Constructor
	 */
	public PDUFTImpl ()
	{
	}
	
	/**
	 * Update Knowladge of the N-1 State
	 */
	public int PDUFTUpdateManager (Event , )
	{}
	
	/**
	 * Recompute forwarding table
	 */
	private int recomputeTable()
	{}
	
	/**
	 * Propagate knowladge of the N-1 state
	 */
	private int propagateKnowladge()
	{}
	
	
}
