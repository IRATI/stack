package rina.ipcmanager.impl;

public class Main {
	
	static {
		System.loadLibrary("rina_java");
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		IPCManager ipcManager = new IPCManager();
		ipcManager.executeEventLoop();
	}

}
