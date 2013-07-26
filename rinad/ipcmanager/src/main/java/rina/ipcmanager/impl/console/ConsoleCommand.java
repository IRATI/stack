package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManagerImpl;

/**
 * Base Console command
 * @author eduardgrasa
 *
 */
public abstract class ConsoleCommand {
	
	private IPCManagerImpl ipcManagerImpl = null;
	
	private String commandId = null;
	
	public ConsoleCommand(String commandId, IPCManagerImpl ipcManagerImpl){
		this.commandId = commandId;
		this.ipcManagerImpl = ipcManagerImpl;
	}
	
	public String getCommandId(){
		return this.commandId;
	}
	
	public IPCManagerImpl getIPCManagerImpl(){
		return this.ipcManagerImpl;
	}
	
	public boolean equals(Object candidate){
		if (candidate == null){
			return false;
		}
		
		if (!(candidate instanceof ConsoleCommand)){
			return false;
		}
		
		ConsoleCommand command = (ConsoleCommand) candidate;
		return command.getCommandId().equals(this.getCommandId());
	}
	
	/**
	 * It has to execute the command and return a String that will be displayed in the console
	 * @param the text entered by the user, separated in words
	 * @return
	 */
	public abstract String execute(String[] splittedCommand);
}
