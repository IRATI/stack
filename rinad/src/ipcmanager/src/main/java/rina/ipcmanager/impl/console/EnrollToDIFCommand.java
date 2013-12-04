package rina.ipcmanager.impl.console;

import java.util.Iterator;

import eu.irati.librina.EnrollToDIFResponseEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.Neighbor;
import eu.irati.librina.TimerExpiredEvent;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class EnrollToDIFCommand extends ConsoleCommand{

	public static final String ID = "enrolldif";
	private static final String USAGE = "enrolldif <ipcp_id> <dif_name> <sup_dif_name> <neigh_ap_name> <neigh_ap_instance>";
	
	private CancelCommandTimerTask cancelCommandTask = null;
	private int ipcProcessId;
	private String difName;
	private String supportingDifName;
	private String neighApName;
	private String neighApInstance;
	
	public EnrollToDIFCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 6){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Integer.parseInt(splittedCommand[1]);
		} catch(Exception ex) {
			return "Could not parse ipcProcessId " + splittedCommand[1];
		}
			
		difName = splittedCommand[2];
		supportingDifName = splittedCommand[3]; 
		neighApName = splittedCommand[4];
		neighApInstance = splittedCommand[5];
		getIPCManagerConsole().lock();
		long handle = -1;
		
		try{
			handle = getIPCManager().requestEnrollmentToDIF(
					ipcProcessId, difName, supportingDifName, neighApName, neighApInstance);
		}catch(Exception ex){
			getIPCManagerConsole().unlock();
			return "Error executing enroll IPC Process to DIF command: " + ex.getMessage();
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
		
		if (!(response instanceof EnrollToDIFResponseEvent)) {
			return "Got a wrong response to an event";
		}
		
		EnrollToDIFResponseEvent event = (EnrollToDIFResponseEvent) response;
		if (event.getResult() == 0) {
			String result = "Successfully enrolled IPC Process " + ipcProcessId + " to DIF " + difName + "\n";
			result = result + "Got the following new neighbors: \n";
			Iterator<Neighbor> iterator = event.getNeighbors().iterator();
			Neighbor neighbor = null;
			while(iterator.hasNext()){
				neighbor = iterator.next();
				result = result + neighbor.getName().getProcessName() 
						 + " " + neighbor.getName().getProcessInstance() + "\n";
			}
			
			return result;
		} else {
			return "Problems enrolling IPC Process " + ipcProcessId + " to DIF " + difName +
					". Error code: " + event.getResult();
		}
	}

}
