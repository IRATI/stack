package rina.ipcmanager.impl.console;

import java.util.Iterator;

import eu.irati.librina.IPCEvent;
import eu.irati.librina.QueryRIBResponseEvent;
import eu.irati.librina.RIBObject;
import eu.irati.librina.TimerExpiredEvent;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class QueryIPCProcessRIBCommand extends ConsoleCommand{

	public static final String ID = "queryrib";
	private static final String USAGE = "queryrib <ipcp_id>";
	
	private CancelCommandTimerTask cancelCommandTask = null;
	private int ipcProcessId;
	
	public QueryIPCProcessRIBCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 2){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Integer.parseInt(splittedCommand[1]);
		} catch(Exception ex) {
			return "Could not parse ipcProcessId " + splittedCommand[1];
		}
		
		getIPCManagerConsole().lock();
		long handle = -1;
		
		try{
			handle = getIPCManager().requestQueryIPCProcessRIB(ipcProcessId);
		}catch(Exception ex){
			getIPCManagerConsole().unlock();
			return "Error executing query IPC Process RIB command: " + ex.getMessage();
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
			return "Error waiting for query IPC Process RIB response: "+ex.getMessage();
		}
		
		if (response == null) {
			return "Got a null response";
		}
		
		if (response instanceof TimerExpiredEvent) {
			return "Error: could not get a reply after " 
					+ CancelCommandTimerTask.INTERVAL_IN_SECONDS + " seconds.";
		}
		
		cancelCommandTask.cancel();
		
		if (!(response instanceof QueryRIBResponseEvent)) {
			return "Got a wrong response to an event";
		}
		
		QueryRIBResponseEvent event = (QueryRIBResponseEvent) response;
		Iterator<RIBObject> iterator = event.getRIBObject().iterator();
		RIBObject ribObject = null;
		String result = "Got successfull response. Contents of the IPC Process RIB: \n";
		while (iterator.hasNext()) {
			ribObject = iterator.next();
			result = result + "Object name: " + ribObject.getName() + "\n";
			result = result + "Object class: " + ribObject.getClazz() + "\n";
			result = result + "Object instance: " + ribObject.getInstance() + "\n";
			//TODO add  object value
			result = result + "\n";
		}
		
		return result;
	}

}
