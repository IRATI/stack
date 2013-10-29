package rina.ipcprocess.impl;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.AssignToDIFRequestEvent;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;
import eu.irati.librina.KernelIPCProcessSingleton;
import eu.irati.librina.rina;

public class IPCProcess {
	
	public enum State {NULL, INITIALIZED, ASSIGN_TO_DIF_IN_PROCESS, ASSIGNED_TO_DIF};
	
	private static final Log log = LogFactory.getLog(IPCProcess.class);
	
	/** To get events from the Kernel IPC Process or the IPC Manager */
	private IPCEventProducerSingleton ipcEventProducer = null;
	
	/** To communicate with the IPC Manager */
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	/** To communicate with the IPC Process components in the kernel */
	private KernelIPCProcessSingleton kernelIPCProcess = null;
	
	/** The state of the IPC Process */ 
	private State state = State.NULL;
	
	/** Map of events pending to be replied */
	private Map<Long, IPCEvent> pendingEvents = null;
	
	/** Information about the dif where the IPC Process is currently assigned */
	private DIFInformation difInformation = null;
	
	public IPCProcess(ApplicationProcessNamingInformation namingInfo, int id, long ipcManagerPort){
		log.info("Initializing librina...");
		
		rina.initialize();
		ipcEventProducer = rina.getIpcEventProducer();
		kernelIPCProcess = rina.getKernelIPCProcess();
		kernelIPCProcess.setIPCProcessId(id);
		ipcManager = rina.getExtendedIPCManager();
		ipcManager.setIpcProcessId(id);
		ipcManager.setIpcProcessName(namingInfo);
		ipcManager.setIPCManagerPort(ipcManagerPort);
		pendingEvents = new ConcurrentHashMap<Long, IPCEvent>();
		
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
		setState(State.INITIALIZED);
		
		log.info("IPC Manager notified successfully");
	}
	
	public State getState() {
		return state;
	}
	
	private void setState(State state) {
		this.state = state;
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
		} else if (event.getType() == IPCEventType.ASSIGN_TO_DIF_REQUEST_EVENT) {
			AssignToDIFRequestEvent asEvent = (AssignToDIFRequestEvent) event;
			processAssignToDIFRequest(asEvent);
		} else if (event.getType() == IPCEventType.ASSIGN_TO_DIF_RESPONSE_EVENT) {
			AssignToDIFResponseEvent asEvent = (AssignToDIFResponseEvent) event;
			processAssignToDIFResponseEvent(asEvent);
		}
	}
	
	private synchronized void processAssignToDIFRequest(AssignToDIFRequestEvent event) throws Exception{
		if (getState() != State.INITIALIZED) {
			//The IPC Process can only be assigned to a DIF once, reply with error message
			log.error("Got a DIF assignment request while not in INITIALIZED state. " 
						+ "Current state is: " + getState());
			ipcManager.assignToDIFResponse(event, -1);
			return;
		}
		
		try {
			long handle = kernelIPCProcess.assignToDIF(event.getDIFInformation());
			pendingEvents.put(handle, event);
			setState(State.ASSIGN_TO_DIF_IN_PROCESS);
		} catch (Exception ex) {
			log.error("Problems sending DIF Assignment request to the kernel: "+ex.getMessage());
			ipcManager.assignToDIFResponse(event, -1);
		}
	}
	
	private synchronized void processAssignToDIFResponseEvent(AssignToDIFResponseEvent event) throws Exception{
		if (getState() != State.ASSIGN_TO_DIF_IN_PROCESS) {
			log.error("Got a DIF assignment response while not in ASSIGN_TO_DIF_IN_CPORCES state." 
						+ " Current state is " + getState());
			return;
		}
		
		IPCEvent ipcEvent = pendingEvents.remove(event.getSequenceNumber());
		if (ipcEvent == null) {
			log.error("Couldn't find an Assign to DIF Request Event associated to the handle " + 
						event.getSequenceNumber());
			return;
		}
		
		if (!(ipcEvent instanceof AssignToDIFRequestEvent)){
			log.error("Expected an Assign To DIF Request Event, but got event of type: " + ipcEvent.getClass().getName());
			return;
		}
		
		AssignToDIFRequestEvent arEvent = (AssignToDIFRequestEvent) ipcEvent;
		if (event.getResult() == 0) {
			log.debug("The kernel processed successfully the Assign to DIF Request");
			ipcManager.assignToDIFResponse(arEvent, 0);
			setState(State.ASSIGNED_TO_DIF);
			difInformation = arEvent.getDIFInformation();
			log.info("IPC Process successfully assigned to DIF "+ difInformation.getDifName());
		} else {
			log.error("The kernel couldn't successfully process the Assign to DIF Request: "+ event.getResult());
			ipcManager.assignToDIFResponse(arEvent, -1);
			setState(State.INITIALIZED);
			log.error("Could not assign IPC Process to DIF " + difInformation.getDifName());
		}
	}

}
