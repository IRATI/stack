package rina.utils.apps.rinaband.utils;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.rina;

public class IPCEventConsumer implements Runnable{
	
	private Map<ApplicationProcessNamingInformation, FlowAcceptor> flowAcceptors;
	private Map<Integer, FlowDeallocationListener> flowDeallocationListeners;
	
	public IPCEventConsumer(){
		flowAcceptors = new ConcurrentHashMap<ApplicationProcessNamingInformation, FlowAcceptor>();
		flowDeallocationListeners = new ConcurrentHashMap<Integer, FlowDeallocationListener>();
	}
	
	public void addFlowAcceptor(FlowAcceptor flowAcceptor, 
			ApplicationProcessNamingInformation localAppName){
		flowAcceptors.put(localAppName, flowAcceptor);
	}
	
	public void removeFlowAcceptor(ApplicationProcessNamingInformation localAppName){
		flowAcceptors.remove(localAppName);
	}
	
	public void addFlowDeallocationListener(FlowDeallocationListener listener, int portId){
		flowDeallocationListeners.put(portId, listener);
	}
	
	public void removeFlowDeallocationListener(int portId){
		flowDeallocationListeners.remove(portId);
	}

	@Override
	public void run() {
		IPCEventProducerSingleton ipcEventProducer = rina.getIpcEventProducer();
		IPCEvent event = null;
		
		while (true){
			event = ipcEventProducer.eventWait();
			System.out.println("IPC Event has happened: "+event.getType());
			
			if (event.getType() == IPCEventType.FLOW_ALLOCATION_REQUESTED_EVENT){
				FlowRequestEvent flowRequestEvent = (FlowRequestEvent) event;
				FlowAcceptor flowAcceptor = flowAcceptors.get(flowRequestEvent.getLocalApplicationName());
				if (flowAcceptor != null){
					flowAcceptor.dispatchFlowRequestEvent(flowRequestEvent);
				}else{
					System.out.println("Could not find handler for flows targetting local application "
								+flowRequestEvent.getLocalApplicationName());
				}
			}else if (event.getType() == IPCEventType.FLOW_DEALLOCATED_EVENT){
				FlowDeallocatedEvent flowDeallocatedEvent = (FlowDeallocatedEvent) event;
				FlowDeallocationListener listener = flowDeallocationListeners.get(flowDeallocatedEvent.getPortId());
				if (listener != null){
					listener.dispatchFlowDeallocatedEvent(flowDeallocatedEvent);
				}else{
					System.out.println("Could not find handler for flow deallocation events targetting flow"
							+flowDeallocatedEvent.getPortId());
				}
			}
		}
	}

}
