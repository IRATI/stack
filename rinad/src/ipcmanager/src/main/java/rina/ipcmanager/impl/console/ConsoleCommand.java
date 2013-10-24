package rina.ipcmanager.impl.console;

import rina.ipcmanager.impl.IPCManager;

/**
 * Base Console command
 * @author eduardgrasa
 *
 */
public abstract class ConsoleCommand {
	
	private IPCManager ipcManager = null;
	
	private IPCManagerConsole ipcManagerConsole = null;
	
	private String commandId = null;
	
	public ConsoleCommand(String commandId, IPCManager ipcManager, 
			IPCManagerConsole ipcManagerConsole){
		this.commandId = commandId;
		this.ipcManager = ipcManager;
		this.ipcManagerConsole = ipcManagerConsole;
	}
	
	public String getCommandId(){
		return this.commandId;
	}
	
	public IPCManager getIPCManager(){
		return this.ipcManager;
	}
	
	public IPCManagerConsole getIPCManagerConsole(){
		return this.ipcManagerConsole;
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
