package rina.ipcprocess.impl;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;
import eu.irati.librina.rina;

public class IPCProcess {

	private static final Log log = LogFactory.getLog(IPCProcess.class);
	
	private IPCEventProducerSingleton ipcEventProducer = null;
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	public IPCProcess(ApplicationProcessNamingInformation namingInfo, int id, long ipcManagerPort){
		log.info("Initializing librina...");
		rina.initialize();
		ipcEventProducer = rina.getIpcEventProducer();
		ipcManager = rina.getExtendedIPCManager();
		ipcManager.setIpcProcessId(id);
		ipcManager.setIpcProcessName(namingInfo);
		ipcManager.setIPCManagerPort(ipcManagerPort);
		log.info("Initialized IPC Process with AP name: "+namingInfo.getProcessName()
				+ "; AP instance: "+namingInfo.getProcessInstance() + "; id: "+id);
		log.info("Notifying IPC Manager about successful initialization... ");
		try {
			ipcManager.notifyIPCProcessInitialized();
		} catch (Exception ex) {
			log.error("Problems notifying IPC Manager about successful initialization: " + ex.getMessage());
			log.error("Shutting down IPC Process");
			System.exit(-1);
		}
		log.info("IPC Manager notified successfully");
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
		
		if (event.getType() == IPCEventType.IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION) {
			IPCProcessDIFRegistrationEvent regEvent = (IPCProcessDIFRegistrationEvent) event;
			if (regEvent.isRegistered()) {
				ipcManager.appRegistered(regEvent.getIPCProcessName(), regEvent.getDIFName());
				log.info("IPC Process registered to N-1 DIF "+ regEvent.getDIFName().getProcessName());
			} else{
				ipcManager.appUnregistered(regEvent.getIPCProcessName(), regEvent.getDIFName());
				log.info("IPC Process unregistered from N-1 DIF "+ regEvent.getDIFName().getProcessName());
			}
		} 
	}

}
