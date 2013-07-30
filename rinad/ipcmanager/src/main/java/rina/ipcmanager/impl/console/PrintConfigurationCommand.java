package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;
import rina.ipcmanager.impl.conf.RINAConfiguration;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class PrintConfigurationCommand extends ConsoleCommand{

	public static final String ID = "printconfig";
	private static final String USAGE = "printconfig";
	
	public PrintConfigurationCommand(IPCManager ipcManager){
		super(ID, ipcManager);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length > 1){
			return "Wrong number of parameters. Usage: "+USAGE;
		}

		return RINAConfiguration.getInstance().toString();
	}

}
