package rina.ipcprocess.impl.enrollment.ribobjects;

import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Neighbor;

import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.configuration.RINAConfiguration;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.events.NeighborDeclaredDeadEvent;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObjectNames;

public class WatchdogRIBObject extends BaseRIBObject implements CDAPMessageHandler{
	
	private static final Log log = LogFactory.getLog(WatchdogRIBObject.class);
	
	public static final String WATCHDOG_OBJECT_NAME = "/dif/management/watchdog";
	public static final String WATCHDOG_OBJECT_CLASS = "watchdog timer";
	
	/** Timer for the watchdog timertask **/
	private Timer timer = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private CDAPMessage responseMessage = null;
	
	private WatchdogTimerTask timerTask = null;
	
	private Map<String, NeighborStatistics> neighborStatistics = null;
	
	/**
	 * The keepalive period in ms.
	 */
	private long periodInMs = 0L;
	
	/**
	 * The declared dead interval in ms
	 */
	private long declaredDeadIntervalInMs = 0L;
	
	private IPCProcess ipcProcess = null;
	
	/**
	 * Schedule the watchdogtimer_task to run every PERIOD_IN_MS milliseconds
	 * @param ipcProcess
	 */
	public WatchdogRIBObject(){
		super(WATCHDOG_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), WATCHDOG_OBJECT_NAME);
		ipcProcess = IPCProcess.getInstance();
		this.cdapSessionManager = ipcProcess.getCDAPSessionManager();
		this.timer = new Timer();
		timerTask = new WatchdogTimerTask(this);
		this.periodInMs = 
				RINAConfiguration.getInstance().getLocalConfiguration().getWatchdogPeriodInMs();
		this.declaredDeadIntervalInMs = 
				RINAConfiguration.getInstance().getLocalConfiguration().getDeclaredDeadIntervalInMs();
		this.neighborStatistics = new ConcurrentHashMap<String, NeighborStatistics>();
		ipcProcess = IPCProcess.getInstance();
		timer.schedule(timerTask, new Double(periodInMs*Math.random()).longValue(), periodInMs);
		setRIBDaemon(ipcProcess.getRIBDaemon());
	}
	
	@Override
	public Object getObjectValue() {
		// TODO Auto-generated method stub
		return null;
	}

	/**
	 * Reply to the CDAP message
	 */
	@Override
	public void read(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
		
		//1 Send M_READ_R message
		try{
			responseMessage = cdapSessionManager.getReadObjectResponseMessage(cdapSessionDescriptor.getPortId(), null, 
					cdapMessage.getObjClass(), 0L, cdapMessage.getObjName(), null, 0, null, cdapMessage.getInvokeID());
			getRIBDaemon().sendMessage(responseMessage, cdapSessionDescriptor.getPortId(), null);
		}catch(Exception ex){
			log.error(ex);
		}
		
		//2 Update the "lastHeardFromTimeInMs" attribute of the neighbor that has issued the read request
		try{
			Neighbor neighbor = (Neighbor) ((NeighborRIBObject) this.getRIBDaemon().read(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + 
					cdapSessionDescriptor.getDestApName())).getObjectValue();
			neighbor.setLastHeardFromTimeInMs(System.currentTimeMillis());
			getRIBDaemon().write(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + 
					neighbor.getName().getProcessName(), 
					neighbor);
		}catch(Exception ex){
			log.error(ex);
		}
	}
	
	/**
	 * Send watchdog messages to the IPC processes that are our neighbors and we're enrolled to
	 */
	protected void sendMessages(){
		this.neighborStatistics.clear();
		
		List<Neighbor> neighbors = ipcProcess.getNeighbors();
		
		CDAPMessage cdapMessage = null;
		NeighborStatistics neighborStats = null;
		long currentTime = System.currentTimeMillis();
		for(int i=0; i<neighbors.size(); i++){
			//Skip non enrolled neighbors
			if (!neighbors.get(i).isEnrolled()){
				continue;
			}
			
			//Skip neighbors that have sent M_READ messages during the last period
			if (neighbors.get(i).getLastHeardFromTimeInMs() + this.periodInMs > currentTime){
				continue;
			}
			
			//If we have not heard from the neighbor during long enough, declare the neighbor
			//dead and fire a NEIGHBOR_DECLARED_DEAD event
			if (neighbors.get(i).getLastHeardFromTimeInMs() != 0 && 
					neighbors.get(i).getLastHeardFromTimeInMs() + this.declaredDeadIntervalInMs < currentTime){
				NeighborDeclaredDeadEvent event = new NeighborDeclaredDeadEvent(neighbors.get(i));
				getRIBDaemon().deliverEvent(event);
				continue;
			}
			
			try{
				cdapMessage = cdapSessionManager.getReadObjectRequestMessage(
						neighbors.get(i).getUnderlyingPortId(), null, null, WatchdogRIBObject.WATCHDOG_OBJECT_CLASS, 
						0, WatchdogRIBObject.WATCHDOG_OBJECT_NAME, 0, true);
				neighborStats = new NeighborStatistics(
						neighbors.get(i).getName().getProcessName(), 
						System.currentTimeMillis());
				neighborStatistics.put(neighborStats.getApName(), neighborStats);
				getRIBDaemon().sendMessage(cdapMessage, neighbors.get(i).getUnderlyingPortId(), this);
			}catch(Exception ex){
				ex.printStackTrace();
				log.error(ex);
			}
		}
	}
	
	/**
	 * Take advantadge of the watchdog message responses to measure the RTT, and store it in 
	 * the neighbor object (average of the last 4 RTTs)
	 */
	public void readResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
		long time = System.currentTimeMillis();
		NeighborStatistics neighborStats = this.neighborStatistics.remove(cdapSessionDescriptor.getDestApName());
		try{
			Neighbor neighbor = (Neighbor) ((NeighborRIBObject) getRIBDaemon().read(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + neighborStats.getApName())).getObjectValue();
			neighbor.setAverageRttInMs(time - neighborStats.getMessageSentTimeInMs());
			neighbor.setLastHeardFromTimeInMs(time);
			log.debug("RTT to "+neighborStats.getApName()+" : "+neighbor.getAverageRttInMs()+" ms");
			getRIBDaemon().write(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + neighborStats.getApName(), 
					neighbor);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}

	public void cancelReadResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}

	public void createResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}

	public void deleteResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}

	public void startResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}

	public void stopResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}

	public void writeResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
		throws RIBDaemonException {
	}
}
