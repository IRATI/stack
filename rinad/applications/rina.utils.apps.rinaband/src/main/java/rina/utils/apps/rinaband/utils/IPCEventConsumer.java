package rina.utils.apps.rinaband.utils;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.rina;

public class IPCEventConsumer implements Runnable{
	
	private static final Log log = LogFactory.getLog(IPCEventConsumer.class);
	private Map<String, FlowAcceptor> flowAcceptors;
	private Map<Integer, FlowDeallocationListener> flowDeallocationListeners;
	
	public IPCEventConsumer(){
		flowAcceptors = new ConcurrentHashMap<String, FlowAcceptor>();
		flowDeallocationListeners = new ConcurrentHashMap<Integer, FlowDeallocationListener>();
	}
	
	public void addFlowAcceptor(FlowAcceptor flowAcceptor, 
			ApplicationProcessNamingInformation localAppName){
		flowAcceptors.put(
				Utils.geApplicationNamingInformationCode(localAppName), 
				flowAcceptor);
		log.info("Added flow acceptor for application: " + 
				Utils.geApplicationNamingInformationCode(localAppName));
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
				log.info("Looking for flow acceptors for application: "
						+ Utils.geApplicationNamingInformationCode(
								flowRequestEvent.getLocalApplicationName()));
				FlowAcceptor flowAcceptor = flowAcceptors.get(
						Utils.geApplicationNamingInformationCode(
						flowRequestEvent.getLocalApplicationName()));
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
