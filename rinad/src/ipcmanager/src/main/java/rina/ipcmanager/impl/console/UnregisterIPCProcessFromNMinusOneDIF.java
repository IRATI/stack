package rina.ipcmanager.impl.console;

import eu.irati.librina.IPCEvent;
import eu.irati.librina.IpcmUnregisterApplicationResponseEvent;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to register an IPC Process to an N-1 DIF
 * @author eduardgrasa
 *
 */
public class UnregisterIPCProcessFromNMinusOneDIF extends ConsoleCommand{

	public static final String ID = "unregn1dif";
	private static final String USAGE = "unregn1dif <ipcp_id> <dif_name>";
	
	private long ipcProcessId;
	private String difName;
	
	public UnregisterIPCProcessFromNMinusOneDIF(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 3){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Long.parseLong(splittedCommand[1]);
		} catch(Exception ex) {
			return "Could not parse ipcProcessId " + splittedCommand[1];
		}
			
		difName = splittedCommand[2];
		getIPCManagerConsole().lock();
		long handle = -1;
		
		try{
			handle = getIPCManager().requestUnregistrationFromNMinusOneDIF(ipcProcessId, difName);
		}catch(Exception ex){
			getIPCManagerConsole().unlock();
			return "Error executing unregister IPC Process from N-1 DIF command: " + ex.getMessage();
		}
		
		getIPCManagerConsole().setPendingRequestId(handle);
		getIPCManagerConsole().unlock();
		IPCEvent response = null;
		
		try{
			response = getIPCManagerConsole().getResponse();
		}catch(Exception ex){
			return "Error waiting for register IPC Process to N-1 DIF response: "+ex.getMessage();
		}
		
		if (response == null) {
			return "Got a null response";
		}
		
		if (!(response instanceof IpcmUnregisterApplicationResponseEvent)) {
			return "Got a wrong response to an event";
		}
		
		IpcmUnregisterApplicationResponseEvent event = (IpcmUnregisterApplicationResponseEvent) response;
		if (event.getResult() == 0) {
			return "Successfully unregistered IPC Process " + ipcProcessId + " from DIF " + difName;
		} else {
			return "Problems unregistering IPC Process " + ipcProcessId + " from DIF " + difName +
					". Error code: " + event.getResult();
		}
	}

}
