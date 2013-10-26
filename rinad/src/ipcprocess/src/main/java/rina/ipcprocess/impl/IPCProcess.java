package rina.ipcprocess.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCProcessRegisteredToDIFEvent;
import eu.irati.librina.rina;

public class IPCProcess {

	private static final Log log = LogFactory.getLog(IPCProcess.class);
	
	/** The IPC Process AP name and AP instance */
	private ApplicationProcessNamingInformation namingInfo = null;
	
	/** The identifier of the IPC Process within the system */
	private int id = 0;
	
	private IPCEventProducerSingleton ipcEventProducer = null;
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	public IPCProcess(ApplicationProcessNamingInformation namingInfo, int id){
		log.info("Initializing librina...");
		rina.initialize();
		this.namingInfo = namingInfo;
		this.id = id;
		ipcEventProducer = rina.getIpcEventProducer();
		ipcManager = rina.getExtendedIPCManager();
		log.info("Initialized IPC Process with AP name: "+namingInfo.getProcessName()
				+ "; AP instance: "+namingInfo.getProcessInstance() + "; id: "+id);
	}
	
	public void executeEventLoop(){
		IPCEvent event = null;
		
		while(true){
			try{
				event = ipcEventProducer.eventWait();
				processEvent(event);
			}catch(Exception ex){
				log.error("Problems processing event of type " + event.getType() + 
						". " + ex.getMessage());
			}
		}
	}
	
	private void processEvent(IPCEvent event) throws Exception{
		log.info("Got event of type: "+event.getType() 
				+ " and sequence number: "+event.getSequenceNumber());
		
		if (event.getType() == IPCEventType.IPC_PROCESS_REGISTERED_TO_DIF) {
			IPCProcessRegisteredToDIFEvent regEvent = (IPCProcessRegisteredToDIFEvent) event;
			ipcManager.commitPendingResitration(0, regEvent.getDIFName());
		}
	}

}
