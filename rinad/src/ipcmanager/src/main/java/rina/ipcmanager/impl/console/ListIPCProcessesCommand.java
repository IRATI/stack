package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class ListIPCProcessesCommand extends ConsoleCommand{

	public static final String ID = "listipcp";
	private static final String USAGE = "listipcp";
	
	public ListIPCProcessesCommand(IPCManager ipcManager, IPCManagerConsole console){
		super(ID, ipcManager, console);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length > 1){
			return "Wrong number of parameters. Usage: "+USAGE;
		}

		return this.getIPCManager().getIPCProcessesInformationAsString();
	}

}
