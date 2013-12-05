package rina.ipcprocess.impl.flowallocator;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.IPCException;
import eu.irati.librina.Neighbor;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionManager;
import rina.ipcprocess.impl.IPCProcess;

/**
 * Different usefull functions
 * @author eduardgrasa
 *
 */
public class Utils {
	
	/**
	 * Maps the destination address to the port id of the N-1 flow that this IPC process can use
	 * to reach the IPC process identified by the address
	 * @return
	 */
	public static synchronized int mapAddressToPortId(long address, IPCProcess ipcProcess) throws IPCException{
		CDAPSessionManager cdapSessionManager = ipcProcess.getCDAPSessionManager();
		List<Neighbor> neighbors = null;
		Neighbor neighbor = null;
		int portId = 0;
		
		try{
			neighbors = ipcProcess.getNeighbors();
			
			for(int i=0; i<neighbors.size(); i++){
				neighbor = neighbors.get(i);
				if (neighbor.getAddress() == address){
					portId = cdapSessionManager.getPortId(neighbor.getName().getProcessName());
					break;
				}
			}
			
			if (portId == 0){
				throw new IPCException("Could not find the application process name of the IPC process whose address is "
										+address);
			}
		}catch(CDAPException ex){
			throw new IPCException(ex.getMessage());
		}
		
		return portId;
	}
	
	/**
	 * Returns the application process name and application process instance of a remote IPC process, given its address.
	 * @param ipcProcessAddress
	 * @param ipcProcess
	 * @return the application process naming information
	 * @throws IPCException if no matching entry is found
	 */
	public static synchronized ApplicationProcessNamingInformation 
		getRemoteIPCProcessNamingInfo(long ipcProcessAddress, IPCProcess ipcProcess) throws IPCException{
		List<Neighbor> neighbors = null;
		Neighbor neighbor = null;

		neighbors = ipcProcess.getNeighbors();

		for(int i=0; i<neighbors.size(); i++){
			neighbor = neighbors.get(i);
			if (neighbor.getAddress() == ipcProcessAddress){
				return neighbor.getName();
			}
		}

		throw new IPCException("Could not find the application process name of the IPC process whose synonym is "
								+ipcProcessAddress);
	}

}
