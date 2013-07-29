package rina.ipcmanager.impl.apservice;

import java.net.Socket;

import rina.cdap.api.message.CDAPMessage;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCService;

/**
 * Contains the information about the state of a flow service request
 * @author eduardgrasa
 *
 */
public class FlowServiceState {
	
	public enum Status {NULL, ALLOCATION_REQUESTED, ALLOCATED, DEALLOCATION_REQUESTED};
	
	/**
	 * The parameters of the requested flow service (source app, destination app, 
	 * QoS parameters, portId)
	 */
	private FlowService flowService = null;
	
	/**
	 * The socket to communicate with the application that is using the flow service
	 */
	private Socket socket = null;
	
	private Status status = Status.NULL;
	
	/**
	 * The IPC Process that is handling our request
	 */
	private IPCService ipcService = null;
	
	/**
	 * The latest relevant CDAP message received from the application
	 */
	private CDAPMessage cdapMessage = null;
	
	/**
	 * The socket reader that reads the messages coming from the application library
	 */
	private TCPSocketReader tcpSocketReader = null;

	public FlowService getFlowService() {
		return flowService;
	}

	public void setFlowService(FlowService flowService) {
		this.flowService = flowService;
	}

	public Socket getSocket() {
		return socket;
	}

	public void setSocket(Socket socket) {
		this.socket = socket;
	}

	public Status getStatus() {
		return status;
	}

	public void setStatus(Status status) {
		this.status = status;
	}

	public void setIpcService(IPCService ipcService) {
		this.ipcService = ipcService;
	}

	public IPCService getIpcService() {
		return ipcService;
	}

	public void setCdapMessage(CDAPMessage cdapMessage) {
		this.cdapMessage = cdapMessage;
	}

	public CDAPMessage getCdapMessage() {
		return cdapMessage;
	}

	public void setTcpSocketReader(TCPSocketReader tcpSocketReader) {
		this.tcpSocketReader = tcpSocketReader;
	}

	public TCPSocketReader getTcpSocketReader() {
		return tcpSocketReader;
	}
	
	@Override
	public boolean equals(Object object){
		if (object == null){
			return false;
		}
		
		if (!(object instanceof FlowServiceState)){
			return false;
		}
		
		FlowServiceState candidate = (FlowServiceState) object;
		
		if (candidate.getFlowService() == null){
			return false;
		}
		
		return (candidate.getFlowService().getPortId() == this.getFlowService().getPortId());
	}
	
}
