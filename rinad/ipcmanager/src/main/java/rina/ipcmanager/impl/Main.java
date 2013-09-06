package rina.ipcmanager.impl;

public class Main {
	
	static {
		System.loadLibrary("rina_java");
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		try{
			IPCManager ipcManager = new IPCManager();
			ipcManager.startEventLoopWorkers();
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}

}
