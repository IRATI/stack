package rina.ipcprocess.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.rina;

public class IPCProcess {

	private static final Log log = LogFactory.getLog(IPCProcess.class);
	
	/** The IPC Process AP name and AP instance */
	private ApplicationProcessNamingInformation namingInfo = null;
	
	/** The identifier of the IPC Process within the system */
	private int id = 0;
	
	public IPCProcess(ApplicationProcessNamingInformation namingInfo, int id){
		this.namingInfo = namingInfo;
		this.id = id;
		log.info("Initializing librina...");
		rina.initialize();
		log.info("InitializedIPC Process with AP name: "+namingInfo.getProcessName()
				+ "; AP instance: "+namingInfo.getProcessInstance() + "; id: "+id);
	}
	
	public void executeEventLoop(){
		while(true){
			
		}
	}

}
