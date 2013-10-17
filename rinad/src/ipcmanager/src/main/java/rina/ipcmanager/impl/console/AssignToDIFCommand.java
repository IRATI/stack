package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class AssignToDIFCommand extends ConsoleCommand{

	public static final String ID = "assigndif";
	private static final String USAGE = "assigndif <ipcp_id> <dif_name>";
	
	private long ipcProcessId;
	private String difName;
	
	public AssignToDIFCommand(IPCManager ipcManager){
		super(ID, ipcManager);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 3){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Long.parseLong(splittedCommand[1]);
			difName = splittedCommand[2];
			getIPCManager().requestAssignToDIF(ipcProcessId, difName);
			return "IPC Process " + ipcProcessId + " successfully assigned to DIF "+difName;
		}catch(Exception ex){
			return "Error executing assign IPC Process to DIF command: " + ex.getMessage();
		}
	}

}
