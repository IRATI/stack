package rina.ipcmanager.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Continuously look for events and process them. Used 
 * to make the IPC Manager multi-threaded
 * @author eduardgrasa
 *
 */
public class IPCManagerEventLoopWorker implements Runnable{

	private static final Log log = LogFactory.getLog(IPCManagerEventLoopWorker.class);
	private IPCManager ipcManager = null;
	
	public IPCManagerEventLoopWorker(IPCManager ipcManager){
		this.ipcManager = ipcManager;
	}

	@Override
	public void run() {
		log.info("IPCManagerEventLoopWorker created and executing the event loop!");
		ipcManager.executeEventLoop();
	}
}
