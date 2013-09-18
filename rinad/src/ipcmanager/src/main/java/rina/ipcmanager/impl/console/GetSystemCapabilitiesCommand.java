package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class GetSystemCapabilitiesCommand extends ConsoleCommand{

	public static final String ID = "getsystemcapabilities";
	private static final String USAGE = "getsystemcapabilities";
	
	public GetSystemCapabilitiesCommand(IPCManager ipcManager){
		super(ID, ipcManager);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length > 1){
			return "Wrong number of parameters. Usage: "+USAGE;
		}

		return getIPCManager().getSystemCapabilitiesAsString();
	}

}
