package rina.ipcmanager.impl.console;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.IPCProcess;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class CreateIPCProcessCommand extends ConsoleCommand{

	public static final String ID = "createipcp";
	private static final String USAGE = "createipcp <process_name> <process_instance> <type>";
	
	ApplicationProcessNamingInformation namingInfo = null;
	private String type = null;
	
	public CreateIPCProcessCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 4){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			namingInfo = new ApplicationProcessNamingInformation();
			namingInfo.setProcessName(splittedCommand[1]);
			namingInfo.setProcessInstance(splittedCommand[2]);
			type = splittedCommand[3];
			IPCProcess ipcProcess = getIPCManager().createIPCProcess(namingInfo, type);
			return "IPC Process " + ipcProcess.getId() + " created successfully.";
		}catch(Exception ex){
			return "Error executing create IPC Process command: " + ex.getMessage();
		}
	}

}
