package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;

/**
 * The command to destroy an existing IPC Process
 * @author eduardgrasa
 *
 */
public class DestroyIPCProcessCommand extends ConsoleCommand{

	public static final String ID = "destroyipcp";
	private static final String USAGE = "destroyipcp <ipcprocessid>";
	
	private long ipcProcessId = 0;
	
	public DestroyIPCProcessCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 2){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Long.parseLong(splittedCommand[1]);
			getIPCManager().destroyIPCProcess(ipcProcessId);
			return "IPC Process " + ipcProcessId + " destroyed successfully.";
		}catch(Exception ex){
			return "Error executing destroy IPC Process command: " + ex.getMessage();
		}
	}

}
