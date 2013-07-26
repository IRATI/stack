package rina.ipcmanager.impl.apservice;

import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;


import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.delimiting.api.Delimiter;
import rina.delimiting.api.DelimiterFactory;
import rina.encoding.api.Encoder;
import rina.idd.api.InterDIFDirectory;
import rina.ipcmanager.api.IPCManager;
import rina.ipcmanager.impl.apservice.FlowServiceState.Status;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcservice.api.APService;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.ApplicationRegistration;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCException;
import rina.ipcservice.api.IPCService;

/**
 * Implements the part of the IPCManager that deals with applications
 * @author eduardgrasa
 *
 */
public class APServiceImpl implements APService{
	private static final Log log = LogFactory.getLog(APServiceImpl.class);
	
	private IPCManager ipcManager = null;

	private InterDIFDirectory interDIFDirectory = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private Encoder encoder = null;
	
	private Delimiter delimiter = null;

	private FlowServiceState flowServiceState = null;
	
	private ApplicationRegistrationState applicationRegistrationState = null;
	
	private SDUDeliveryService sduDeliveryService = null;
	
	public APServiceImpl(IPCManager ipcManager, SDUDeliveryService sduDeliveryService){
		this.ipcManager = ipcManager;
		this.sduDeliveryService = sduDeliveryService;
		cdapSessionManager = ipcManager.getCDAPSessionManagerFactory().createCDAPSessionManager();
		encoder = ipcManager.getEncoderFactory().createEncoderInstance();
		delimiter = ipcManager.getDelimiterFactory().createDelimiter(DelimiterFactory.DIF);
	}
	
	public void setInterDIFDirectory(InterDIFDirectory interDIFDirectory){
		this.interDIFDirectory = interDIFDirectory;
	}
	
	public void setFlowServiceState(FlowServiceState flowServiceState){
		this.flowServiceState = flowServiceState;
	}
	
	/**
	 * This operation will query the IDD to see what DIF is the one through which the destination application process 
	 * is available. After that, it will find the IPC process that belongs to that DIF, and trigger a flow allocation 
	 * request.
	 * @param flowService
	 * @param cdapMessage
	 * @param socket
	 */
	public void processAllocateRequest(FlowService flowService, CDAPMessage cdapMessage, Socket socket, TCPSocketReader tcpSocketReader){
		//1 check that there isn't a flow already allocated or in the process of being allocated
		if (flowServiceState != null){
			CDAPMessage errorMessage = cdapMessage.getReplyMessage();
			errorMessage.setResult(1);
			errorMessage.setResultReason("A flow is already allocated or being allocated, cannot allocate another one.");
			sendErrorMessageAndCloseSocket(errorMessage, socket);
			return;
		}
		
		//TODO should ask the IDD, but we don't have one yet, and there can be a single DIF per system only
		
		//Look for the local IPC Process that is a member of difName
		List<IPCProcess> ipcProcesses = ipcManager.listIPCProcesses();
		IPCService ipcService = null;
		if (ipcProcesses == null || ipcProcesses.size() == 0){
			CDAPMessage errorMessage = cdapMessage.getReplyMessage();
			errorMessage.setResult(1);
			errorMessage.setResultReason("Could not find an IPC Process in this system");
			sendErrorMessageAndCloseSocket(errorMessage, socket);
			return;
		}else{
			for(int i=0; i<ipcProcesses.size(); i++){
				if (ipcProcesses.get(i).getType() == IPCProcess.IPCProcessType.NORMAL){
					ipcService = (IPCService) ipcProcesses.get(i);
					break;
				}
			}
		}
		
		this.flowServiceState = new FlowServiceState();
		
		synchronized(this.flowServiceState){
			//Once we have the IPCService, invoke allocate request
			try{
				int portId = ipcService.submitAllocateRequest(flowService, this);
				tcpSocketReader.setPortId(portId);
			}catch(IPCException ex){
				ex.printStackTrace();
				CDAPMessage errorMessage = cdapMessage.getReplyMessage();
				errorMessage.setResult(1);
				errorMessage.setResultReason(ex.getMessage());
				sendErrorMessageAndCloseSocket(errorMessage, socket);
				return;
			}

			//Store the state of the requested flow service
			flowServiceState.setFlowService(flowService);
			flowServiceState.setSocket(socket);
			flowServiceState.setIpcService(ipcService);
			flowServiceState.setCdapMessage(cdapMessage);
			flowServiceState.setTcpSocketReader(tcpSocketReader);
			flowServiceState.setStatus(Status.ALLOCATION_REQUESTED);
		}
	}
	
	/**
	 * Just call the IPC process to release the flow associated to the socket, if it was not already done.
	 * @param portId
	 * @param apNamingInfo True if this socket was being used as an application registration
	 */
	public synchronized void processSocketClosed(int portId, ApplicationProcessNamingInfo apNamingInfo){
		if (apNamingInfo == null){
			if (flowServiceState == null){
				return;
			}

			if (!flowServiceState.getStatus().equals(Status.ALLOCATED)){
				return;
			}

			try{
				this.flowServiceState.getIpcService().submitDeallocate(portId);
			}catch(Exception ex){
				ex.printStackTrace();
				//TODO, what to do?
			}
			
			this.sduDeliveryService.removeSocketToPortIdMapping(portId);
		}else{

			if (this.applicationRegistrationState == null){
				return;
			}
			
			List<String> difNames = null;
			if (this.applicationRegistrationState.getApplicationRegistration().getDifNames() == null || 
					this.applicationRegistrationState.getApplicationRegistration().getDifNames().size() == 0){
				difNames = ipcManager.listDIFNames();
			}else{
				difNames = this.applicationRegistrationState.getApplicationRegistration().getDifNames();
			}
			
			for(int i=0; i<difNames.size(); i++){
				IPCService ipcService = (IPCService) ipcManager.getIPCProcessBelongingToDIF(difNames.get(i));
				if (ipcService != null){
					try{
						ipcService.unregister(apNamingInfo);
					}catch(IPCException ex){
						ex.printStackTrace();
						log.warn("Error when unregistering application from DIF " + difNames.get(i) + ". "+ex.getMessage());
					}
				}
			}
			
			this.applicationRegistrationState = null;
			log.info("Application "+apNamingInfo.getEncodedString()+" canceled the registration with DIF(s) "+printStringList(difNames));
		}
	}
	
	/**
	 * Register the application to one or more DIFs available in this system. The application will open a 
	 * server socket at a certain port and listen for connection attempts.
	 * @param applicationRegistration
	 * @param cdapMessage
	 * @param socket
	 * @param tcpSocketReader
	 */
	public void processApplicationRegistrationRequest(ApplicationRegistration applicationRegistration, 
			CDAPMessage cdapMessage, Socket socket){
		if(this.applicationRegistrationState != null){
			CDAPMessage errorMessage = cdapMessage.getReplyMessage();
			errorMessage.setResult(1);
			errorMessage.setResultReason("The application is already registered through this socket");
			sendMessage(errorMessage, socket);
			return;
		}
		
		
		List<String> difNames = new ArrayList<String>();
		/*if (applicationRegistration.getDifNames() == null || applicationRegistration.getDifNames().size() == 0){
			difNames = ipcManager.listDIFNames();
		}else{
			difNames = applicationRegistration.getDifNames();
		}

		//Register the application in the required DIFs
		for(int i=0; i<difNames.size(); i++){
			IPCService ipcService = (IPCService) ipcManager.getIPCProcessBelongingToDIF(difNames.get(i));
			if (ipcService == null){
				//TODO error message, one of the DIF names is not available in the system
				//return?
			}else{
				try{
					ipcService.register(applicationRegistration.getApNamingInfo(), this);
				}catch(Exception ex){
					//TODO handle well this exception
				}
			}
		}*/
		
		List<IPCProcess> ipcProcesses = ipcManager.listIPCProcesses();
		IPCProcess ipcProcess = null;
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType() == IPCProcess.IPCProcessType.NORMAL){
				try{
					((IPCService) ipcProcess).register(applicationRegistration.getApNamingInfo(), this);
					difNames.add(ipcProcess.getDIFName());
				}catch(Exception ex){
					log.error("Error registering application", ex);
				}
			}
		}

		//Reply to the application
		try{
			CDAPMessage confirmationMessage = cdapMessage.getReplyMessage();
			sendMessage(confirmationMessage, socket);
		}catch(Exception ex){
			ex.printStackTrace();
			//TODO what to do?
		}

		//Update the state
		this.applicationRegistrationState = new ApplicationRegistrationState(applicationRegistration);
		this.applicationRegistrationState.setSocket(socket);
		
		log.info("Application "+applicationRegistration.getApNamingInfo().getEncodedString() +
				" registered to DIF(s) "+printStringList(difNames));
	}

	/**
	 * Another application is trying to establish a flow to the application identified by 
	 * the application naming information within the FlowService object. This operation will look at the application 
	 * and see if it is registered. If it is, it will send an M_CREATE message to it, and wait for the response. If not,
	 * it will call the ipcService with a negative response. It could try to instantiate the destination application, 
	 * but this is not implemented right now.
	 * @param FlowService flowService
	 * @param IPCService the ipcService to call back
	 * @return string if there was no error it is null. If the IPC Manager could not find the
	 * destination application or something else bad happens, it will return a string detailing the error 
	 * (then the callback will never be invoked back)
	 */
	public String deliverAllocateRequest(FlowService flowService, IPCService ipcService){
		if (this.applicationRegistrationState == null){
			return "The destination application process is not registered";
		}
		
		//TODO, check if the request is coming from a DIF where the application is registered
		
		//Connect to the destination application process
		try{
			Socket socket = new Socket("localhost", this.applicationRegistrationState.getApplicationRegistration().getSocketNumber());
			APServiceImpl apServiceImpl = new APServiceImpl(this.ipcManager, this.sduDeliveryService);
			apServiceImpl.setInterDIFDirectory(this.interDIFDirectory);
			FlowServiceState flowServiceState = new FlowServiceState();
			flowServiceState.setFlowService(flowService);
			flowServiceState.setSocket(socket);
			flowServiceState.setIpcService(ipcService);
			flowServiceState.setStatus(Status.ALLOCATION_REQUESTED);
			apServiceImpl.setFlowServiceState(flowServiceState);
			TCPSocketReader socketReader = new TCPSocketReader(socket, ipcManager.getDelimiterFactory().createDelimiter(DelimiterFactory.DIF),
					ipcManager.getEncoderFactory().createEncoderInstance(), ipcManager.getCDAPSessionManagerFactory().createCDAPSessionManager(), 
					apServiceImpl);
			socketReader.setPortId(flowService.getPortId());
			ipcManager.execute(socketReader);
			
			byte[] encodedObject = encoder.encode(flowService);
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(encodedObject);
			CDAPMessage cdapMessage = CDAPMessage.
				getCreateObjectRequestMessage(null, null, FlowService.OBJECT_CLASS, 0, FlowService.OBJECT_NAME, objectValue, 0);
			sendMessage(cdapMessage, socket);
		}catch(Exception ex){
			ex.printStackTrace();
			return ex.getMessage();
		}
		
		return null;
	}

	/**
	 * Invoked when the application process has sent an M_CREATE_R in response to an
	 * allocation request
	 * @param cdapMessage
	 * @param portId
	 */
	public synchronized void processAllocateResponse(CDAPMessage cdapMessage, int portId, TCPSocketReader socketReader){
		if (this.flowServiceState == null || 
				!this.flowServiceState.getStatus().equals(Status.ALLOCATION_REQUESTED)){
			//TODO, what to do? just send error message?
			log.error("Could not find the state of the flow associated to portId "+portId);
			return;
		}
		
		if (cdapMessage.getResult() == 0){
			//Flow allocation accepted
			try{
				flowServiceState.getIpcService().submitAllocateResponse(portId, true, null, this);
				flowServiceState.setStatus(Status.ALLOCATED);
				socketReader.setIPCManager(this.ipcManager);
				sduDeliveryService.addSocketToPortIdMapping(flowServiceState.getSocket(), portId);
				ipcManager.getIncomingFlowQueue(portId).subscribeToQueueReadyToBeReadEvents(sduDeliveryService);
			}catch(Exception ex){
				ex.printStackTrace();
				//TODO what to do?
			}
		}else{
			//Flow allocation denied
			try{
				flowServiceState.getIpcService().submitAllocateResponse(portId, false, cdapMessage.getResultReason(), this);
				Socket socket = flowServiceState.getSocket();
				if (socket.isConnected()){
					socket.close();
				}
			}catch(Exception ex){
				ex.printStackTrace();
				//TODO what to do?
			}
		}
	}
	
	/**
	 * This primitive is invoked by the IPC process to the IPC Manager
	 * to indicate the success or failure of the request associated with this port-id. 
	 * @param port_id -1 if error, portId otherwise
	 * @param result errorCode if result > 0, ok otherwise
	 * @param resultReason null if no error, error description otherwise
	 */
	public void deliverAllocateResponse(int portId, int result, String resultReason){
		if (this.flowServiceState == null){
			log.warn("Received an allocate response for portid " + portId + ", but didn't have any pending allocation request identified by this portId");
			return;
		}
		
		synchronized(this.flowServiceState){
			if (!this.flowServiceState.getStatus().equals(FlowServiceState.Status.ALLOCATION_REQUESTED)){
				//TODO, what to do?
				return;
			}

			switch(result){
			case 0:
				try{
					flowServiceState.setStatus(Status.ALLOCATED);
					flowServiceState.getTcpSocketReader().setIPCManager(this.ipcManager);
					sduDeliveryService.addSocketToPortIdMapping(flowServiceState.getSocket(), portId);
					ipcManager.getIncomingFlowQueue(portId).subscribeToQueueReadyToBeReadEvents(sduDeliveryService);
					byte[] encodedValue = encoder.encode(flowServiceState.getFlowService());
					ObjectValue objectValue = new ObjectValue();
					objectValue.setByteval(encodedValue);
					CDAPMessage confirmationMessage = flowServiceState.getCdapMessage().getReplyMessage();
					confirmationMessage.setObjValue(objectValue);
					confirmationMessage.setResult(result);
					confirmationMessage.setResultReason(resultReason);
					sendMessage(confirmationMessage, flowServiceState.getSocket());
				}catch(Exception ex){
					ex.printStackTrace();
					//TODO what to do?
				}
				break;
			default:
				CDAPMessage errorMessage = flowServiceState.getCdapMessage().getReplyMessage();
				errorMessage.setResult(result);
				errorMessage.setResultReason(resultReason);
				sendErrorMessageAndCloseSocket(errorMessage, flowServiceState.getSocket());
			}
		}
	}
	
	/**
	 * Invoked when a Delete_Flow primitive invoked by an IPC process
	 * @param request
	 */
	public synchronized void deliverDeallocate(int portId) {
		if (this.flowServiceState == null){
			log.warn("Received a deallocate for portid " + portId + ", but didn't find the state associated to this portId");
			return;
		}

		if (!flowServiceState.getStatus().equals(FlowServiceState.Status.ALLOCATED)){
			//TODO, what to do?
			return;
		}

		try{
			this.flowServiceState.getSocket().close();
			this.flowServiceState = null;
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	/**
	 * Send a message to the application library
	 * @param cdapMessage
	 * @param socket
	 */
	public void sendMessage(CDAPMessage cdapMessage, Socket socket){
		try{
			sendCDAPMessage(cdapMessage, socket);
		}catch(Exception ex){
			ex.printStackTrace();
			//TODO what to do?
		}
	}
	
	/**
	 * Send a CDAP reply message to the application library, and after that close the 
	 * socket.
	 * @param message
	 * @param socket
	 */
	public void sendErrorMessageAndCloseSocket(CDAPMessage cdapMessage, Socket socket){
		log.error(cdapMessage.getResultReason());
		try{
			sendCDAPMessage(cdapMessage, socket);
		}catch(Exception ex){
			ex.printStackTrace();
		}finally{
			try{
				socket.close();
			}catch(IOException ex){
				ex.printStackTrace();
			}
		}
	}
	
	private void sendCDAPMessage(CDAPMessage cdapMessage, Socket socket) throws CDAPException, IOException{
		socket.getOutputStream().write(
				delimiter.getDelimitedSdu(
						cdapSessionManager.encodeCDAPMessage(
								cdapMessage)));
	}

	public void deliverStatus(int arg0, boolean arg1) {
		// TODO Auto-generated method stub
		
	}
	
	private String printStringList(List<String> list){
		String result = "";
		if (list == null){
			return "";
		}
		
		for(int i=0; i<list.size(); i++){
			result = result + list.get(i);
			if (i+1<list.size()){
				result = result + ", ";
			}
		}
		
		return result;
	}

}
