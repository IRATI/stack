package rina.ipcmanager.impl.console;

import eu.irati.librina.DIFConfiguration;
import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class UpdateDIFConfigurationCommand extends ConsoleCommand{

	public static final String ID = "updatedifconf";
	private static final String USAGE = "updatedifconf <ipcp_id>";
	
	private int ipcProcessId;
	
	public UpdateDIFConfigurationCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 2){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		try{
			ipcProcessId = Integer.parseInt(splittedCommand[1]);
			getIPCManager().requestUpdateDIFConfiguration(ipcProcessId, 
					new DIFConfiguration());
			return "Modified configuraiton of " + ipcProcessId + " successfully";
		}catch(Exception ex){
			return "Error executing update DIF configuration command: " + ex.getMessage();
		}
	}

}
