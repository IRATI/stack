package rina.utils.apps.echo.utils;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.RegisterApplicationResponseEvent;
import eu.irati.librina.UnregisterApplicationResponseEvent;
import eu.irati.librina.rina;

public class IPCEventConsumer implements Runnable{
	
	private static final Log log = LogFactory.getLog(IPCEventConsumer.class);
	private Map<String, FlowAcceptor> flowAcceptors;
	private Map<Integer, FlowDeallocationListener> flowDeallocationListeners;
	private Map<String, ApplicationRegistrationListener> applicationRegistrationListeners;
	private Map<Long, FlowAllocationListener> flowAllocationListeners;
	
	public IPCEventConsumer(){
		flowAcceptors = new ConcurrentHashMap<String, FlowAcceptor>();
		flowDeallocationListeners = new ConcurrentHashMap<Integer, FlowDeallocationListener>();
		applicationRegistrationListeners = 
				new ConcurrentHashMap<String, ApplicationRegistrationListener>();
		flowAllocationListeners = new ConcurrentHashMap<Long, FlowAllocationListener>();
	}
	
	public void addFlowAcceptor(FlowAcceptor flowAcceptor, 
			ApplicationProcessNamingInformation localAppName){
		flowAcceptors.put(
				Utils.getApplicationNamingInformationCode(localAppName), 
				flowAcceptor);
		log.info("Added flow acceptor for application: " + 
				Utils.getApplicationNamingInformationCode(localAppName));
	}
	
	public void removeFlowAcceptor(ApplicationProcessNamingInformation localAppName){
		flowAcceptors.remove(Utils.getApplicationNamingInformationCode(localAppName));
	}
	
	public void addFlowDeallocationListener(FlowDeallocationListener listener, int portId){
		flowDeallocationListeners.put(portId, listener);
	}
	
	public void removeFlowDeallocationListener(int portId){
		flowDeallocationListeners.remove(portId);
	}
	
	public void addApplicationRegistrationListener(ApplicationRegistrationListener listener, 
			ApplicationProcessNamingInformation appName){
		applicationRegistrationListeners.put(Utils.
				getApplicationNamingInformationCode(appName),listener);
	}
	
	public void removeApplicationRegistrationListener(ApplicationProcessNamingInformation appName){
		applicationRegistrationListeners.remove(
				Utils.getApplicationNamingInformationCode(appName));
	}
	
	public void addFlowAllocationListener(FlowAllocationListener listener, 
			long handle){
		flowAllocationListeners.put(handle, listener);
	}
	
	public void removeFlowAllocationListener(long handle){
		flowAllocationListeners.remove(handle);
	}
	
	@Override
	public void run() {
		IPCEventProducerSingleton ipcEventProducer = rina.getIpcEventProducer();
		IPCEvent event = null;
		
		while (true){
			event = ipcEventProducer.eventWait();
			System.out.println("IPC Event has happened: "+event.getEventType());
			
			if (event.getEventType() == IPCEventType.FLOW_ALLOCATION_REQUESTED_EVENT){
				FlowRequestEvent flowRequestEvent = (FlowRequestEvent) event;
				log.info("Looking for flow acceptors for application: "
						+ Utils.getApplicationNamingInformationCode(
								flowRequestEvent.getLocalApplicationName()));
				FlowAcceptor flowAcceptor = flowAcceptors.get(
						Utils.getApplicationNamingInformationCode(
						flowRequestEvent.getLocalApplicationName()));
				if (flowAcceptor != null){
					flowAcceptor.dispatchFlowRequestEvent(flowRequestEvent);
				}else{
					System.out.println("Could not find handler for flows targetting local application "
								+flowRequestEvent.getLocalApplicationName());
				}
			} else if (event.getEventType() == IPCEventType.FLOW_DEALLOCATED_EVENT){
				FlowDeallocatedEvent flowDeallocatedEvent = (FlowDeallocatedEvent) event;
				FlowDeallocationListener listener = flowDeallocationListeners.get(flowDeallocatedEvent.getPortId());
				if (listener != null){
					listener.dispatchFlowDeallocatedEvent(flowDeallocatedEvent);
				}else{
					System.out.println("Could not find handler for flow deallocation events targetting flow"
							+flowDeallocatedEvent.getPortId());
				}
			} else if (event.getEventType() == IPCEventType.REGISTER_APPLICATION_RESPONSE_EVENT){
				RegisterApplicationResponseEvent regEvent = (RegisterApplicationResponseEvent) event;
				ApplicationRegistrationListener listener = applicationRegistrationListeners.get(
						Utils.getApplicationNamingInformationCode(regEvent.getApplicationName()));
				if (listener != null){
					listener.dispatchApplicationRegistrationResponseEvent(regEvent);
				}
			} else if (event.getEventType() == IPCEventType.UNREGISTER_APPLICATION_RESPONSE_EVENT){
				UnregisterApplicationResponseEvent regEvent = (UnregisterApplicationResponseEvent) event;
				ApplicationRegistrationListener listener = applicationRegistrationListeners.get(
						Utils.getApplicationNamingInformationCode(regEvent.getApplicationName()));
				if (listener != null){
					listener.dispatchApplicationUnregistrationResponseEvent(regEvent);
				}
			} else if (event.getEventType() == IPCEventType.ALLOCATE_FLOW_REQUEST_RESULT_EVENT){
				AllocateFlowRequestResultEvent flowEvent = (AllocateFlowRequestResultEvent) event;
				FlowAllocationListener listener = flowAllocationListeners.get(event.getSequenceNumber());
				if (listener != null){
					listener.dispatchAllocateFlowRequestResultEvent(flowEvent);
				}
			}
		}
	}

}
