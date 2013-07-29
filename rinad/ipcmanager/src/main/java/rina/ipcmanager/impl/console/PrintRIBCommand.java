package rina.ipcmanager.impl.console;

import java.util.List;

import rina.ipcmanager.impl.IPCManagerImpl;

/**
 * The command to create a new IPC Process
 * @author eduardgrasa
 *
 */
public class PrintRIBCommand extends ConsoleCommand{

	public static final String ID = "printrib";
	private static final String USAGE = "printrib apName apInstance ";
	
	/**
	 * Required parameter
	 */
	private String applicationProcessName = null;
	
	/**
	 * Required parameter
	 */
	private String applicationProcessInstance = null;
	
	public PrintRIBCommand(IPCManagerImpl ipcManagerImpl){
		super(ID, ipcManagerImpl);
	}
	
	@Override
	public String execute(String[] splittedCommand) {
		if (splittedCommand.length != 3){
			return "Wrong number of parameters. Usage: "+USAGE;
		}
		
		applicationProcessName = splittedCommand[1];
		applicationProcessInstance = splittedCommand[2];
		
		try{
			List<String> rib = this.getIPCManagerImpl().getPrintedRIB(applicationProcessName, applicationProcessInstance);
			String result = "RIB of IPC process " + applicationProcessName + ": \n";
			for(int i=0; i<rib.size(); i++){
				result = result + rib.get(i);
			}
			return result;
		}catch(Exception ex){
			ex.printStackTrace();
			return "Problems printing the RIB of the IPC Process. " +ex.getMessage();
		}
	}

}
