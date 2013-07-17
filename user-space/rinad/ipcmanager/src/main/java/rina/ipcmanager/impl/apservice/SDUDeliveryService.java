package rina.ipcmanager.impl.apservice;

import java.net.Socket;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.aux.QueueReadyToBeReadSubscriptor;
import rina.delimiting.api.Delimiter;
import rina.delimiting.api.DelimiterFactory;
import rina.ipcmanager.api.IPCManager;

/**
 * Reads relevant incoming flow queues and delivers 
 * the SDUs to applications
 * @author eduardgrasa
 *
 */
public class SDUDeliveryService implements QueueReadyToBeReadSubscriptor, Runnable {
	
	private static final Log log = LogFactory.getLog(SDUDeliveryService.class);
	
	/**
	 * The portId to socket mappings
	 */
	private Map<Integer, Socket> portIdToSocketMappings = null;
	
	/**
	 * The IPC Manager
	 */
	private IPCManager ipcManager = null;
	
	/**
	 * The queues from incoming flows
	 */
	private BlockingQueue<Integer> queuesReadyToBeRead = null;
	
	/**
	 * The delimiter
	 */
	private Delimiter delimiter = null;

	public SDUDeliveryService(IPCManager ipcManager){
		this.queuesReadyToBeRead = new LinkedBlockingQueue<Integer>();
		this.ipcManager = ipcManager;
		this.portIdToSocketMappings = new ConcurrentHashMap<Integer, Socket>();
		log.debug("SDU Delivery Service executing!");
	}
	
	public void addSocketToPortIdMapping(Socket socket, int portId){
		this.portIdToSocketMappings.put(new Integer(portId), socket);
	}
	
	public void removeSocketToPortIdMapping(int portId){
		this.portIdToSocketMappings.remove(new Integer(portId));
	}
	
	private Delimiter getDelimiter(){
		if (delimiter == null){
			delimiter = ipcManager.getDelimiterFactory().createDelimiter(DelimiterFactory.DIF);
		}
		
		return delimiter;
	}
	
	public void run(){
		int portId = -1;
		byte[] sdu = null;
		Socket socket = null;
		
		Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
		
		while(true){
			try{
				portId = this.queuesReadyToBeRead.take().intValue();
				sdu = this.ipcManager.getIncomingFlowQueue(portId).take();
				socket = this.portIdToSocketMappings.get(new Integer(portId));
				
				if (socket == null){
					log.error("Received data from portid " + portId + ", but didn't find the socket to contact the application on this port");
				}
				
				socket.getOutputStream().write(getDelimiter().getDelimitedSdu(sdu));
			}catch(Exception ex){
				log.error(ex);
			}
		}
	}
	
	public void queueReadyToBeRead(int queueId, boolean inputOutput) {
		try {
			this.queuesReadyToBeRead.put(new Integer(queueId));
		} catch (InterruptedException e) {
			log.error(e);
		}
	}

}
