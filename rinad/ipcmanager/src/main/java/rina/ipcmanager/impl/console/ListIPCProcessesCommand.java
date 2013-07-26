package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManagerImpl;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class ListIPCProcessesCommand extends ConsoleCommand{

	public static final String ID = "listipcprocesses";
	private static final String USAGE = "listipcprocesses";
	
	public ListIPCProcessesCommand(IPCManagerImpl ipcManagerImpl){
		super(ID, ipcManagerImpl);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length > 1){
			return "Wrong number of parameters. Usage: "+USAGE;
		}

		return this.getIPCManagerImpl().listIPCProcessesInformation();
	}

}
