package rina.ipcmanager.impl.console;

import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.TimerExpiredEvent;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class AssignToDIFCommand extends ConsoleCommand{

	public static final String ID = "assigndif";
	private static final String USAGE = "assigndif <ipcp_id> <dif_name>";
	
	private CancelCommandTimerTask cancelCommandTask = null;
	private int ipcProcessId;
	private String difName;
	
	public AssignToDIFCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 3){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Integer.parseInt(splittedCommand[1]);
		} catch(Exception ex) {
			return "Could not parse ipcProcessId " + splittedCommand[1];
		}
			
		difName = splittedCommand[2];
		getIPCManagerConsole().lock();
		long handle = -1;
		
		try{
			handle = getIPCManager().requestAssignToDIF(ipcProcessId, difName);
		}catch(Exception ex){
			getIPCManagerConsole().unlock();
			return "Error executing assign IPC Process to DIF command: " + ex.getMessage();
		}
		
		getIPCManagerConsole().setPendingRequestId(handle);
		cancelCommandTask = new CancelCommandTimerTask(getIPCManagerConsole(), handle);
		getIPCManagerConsole().scheduleTask(cancelCommandTask, 
				CancelCommandTimerTask.INTERVAL_IN_SECONDS*1000);
		getIPCManagerConsole().unlock();
		IPCEvent response = null;
		
		try{
			response = getIPCManagerConsole().getResponse();
		}catch(Exception ex){
			return "Error waiting for assign to DIF response: "+ex.getMessage();
		}
		
		if (response == null) {
			return "Got a null response";
		}
		
		if (response instanceof TimerExpiredEvent) {
			return "Error: could not get a reply after " 
					+ CancelCommandTimerTask.INTERVAL_IN_SECONDS + " seconds.";
		}
		
		cancelCommandTask.cancel();
		
		if (!(response instanceof AssignToDIFResponseEvent)) {
			return "Got a wrong response to an event";
		}
		
		AssignToDIFResponseEvent event = (AssignToDIFResponseEvent) response;
		if (event.getResult() == 0) {
			return "Successfully assigned IPC Process " + ipcProcessId + " to DIF " + difName;
		} else {
			return "Problems assigning IPC Process " + ipcProcessId + " to DIF " + difName +
					". Error code: " + event.getResult();
		}
	}

}
