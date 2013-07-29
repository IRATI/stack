package rina.ipcmanager.impl.test;

import java.net.Socket;

import junit.framework.Assert;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.delimiting.api.BaseSocketReader;
import rina.delimiting.api.Delimiter;
import rina.encoding.api.Encoder;
import rina.ipcservice.api.FlowService;

public class SocketReader extends BaseSocketReader{
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private IPCManagerAppInteractionTest test = null;
	
	private CDAPMessage lastMessage = null;
	
	private Encoder encoder = null;
	
	public SocketReader(Socket socket, Delimiter delimiter, CDAPSessionManager cdapSessionManager, IPCManagerAppInteractionTest test){
		super(socket, delimiter);
		this.cdapSessionManager = cdapSessionManager;
		this.test = test;
	}
	
	/**
	 * process the pdu that has been found
	 * @param pdu
	 */
	public void processPDU(byte[] pdu){
		try{
			CDAPMessage cdapMessage = cdapSessionManager.decodeCDAPMessage(pdu);
			switch(cdapMessage.getOpCode()){
			case M_CREATE:
				System.out.println(cdapMessage);
				try{
					FlowService flowService = (FlowService) encoder.decode(cdapMessage.getObjValue().getByteval(), FlowService.class);
					Assert.assertEquals(flowService.getPortId(), 24);
					Assert.assertEquals(flowService.getSourceAPNamingInfo().getApplicationProcessName(), "B");
					Assert.assertEquals(flowService.getDestinationAPNamingInfo().getApplicationProcessName(), "A");
					
					CDAPMessage replyMessage = cdapMessage.getReplyMessage();
					byte[] encodedMessage = cdapSessionManager.encodeCDAPMessage(replyMessage);
					byte[] delimitedMessage = this.getDelimiter().getDelimitedSdu(encodedMessage);
					getSocket().getOutputStream().write(delimitedMessage);
				}catch(Exception ex){
					ex.printStackTrace();
				}
				lastMessage = cdapMessage;
				break;
			case M_CREATE_R:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				break;
			case M_WRITE:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				break;
			case M_DELETE:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				try{
					CDAPMessage replyMessage = cdapMessage.getReplyMessage();
					byte[] encodedMessage = cdapSessionManager.encodeCDAPMessage(replyMessage);
					byte[] delimitedMessage = this.getDelimiter().getDelimitedSdu(encodedMessage);
					getSocket().getOutputStream().write(delimitedMessage);
				}catch(Exception ex){
					ex.printStackTrace();
				}
				break;
			case M_DELETE_R:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				break;
			case M_START_R:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				break;
			case M_STOP_R:
				System.out.println(cdapMessage.toString());
				lastMessage = cdapMessage;
				break;
			default:
				//TODO
			}
		}catch(Exception ex){
			ex.printStackTrace();
		}
		
	}
	
	public CDAPMessage getLastMessage(){
		return lastMessage;
	}
	
	/**
	 * Invoked when the socket is disconnected
	 */
	public void socketDisconnected(){
		System.out.println("Socket disconnected!!");
	}

	public void setEncoder(Encoder encoder) {
		this.encoder = encoder;
	}

	public Encoder getEncoder() {
		return encoder;
	}

}
