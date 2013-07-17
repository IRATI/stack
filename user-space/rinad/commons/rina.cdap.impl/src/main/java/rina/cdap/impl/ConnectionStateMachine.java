package rina.cdap.impl;

import java.util.Timer;
import java.util.TimerTask;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Implements the CDAP connection state machine
 * @author eduardgrasa
 *
 */
public class ConnectionStateMachine {
	private enum ConnectionState {NULL, AWAITCON, CONNECTED, AWAITCLOSE};
	
	private static final Log log = LogFactory.getLog(ConnectionStateMachine.class);
	
	/**
	 * The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	 */
	private long timeout = 0;
	
	/**
	 * The flow that this CDAP connection operates over
	 */
	private CDAPSessionImpl cdapSession = null;
	
	/**
	 * The state of the CDAP connection, drives the CDAP connection 
	 * state machine
	 */
	private ConnectionState connectionState = ConnectionState.NULL;
	
	private Timer openTimer = null;
	
	private Timer closeTimer = null;
	
	public ConnectionStateMachine(CDAPSessionImpl cdapSession, long timeout){
		this.cdapSession = cdapSession;
		this.timeout = timeout;
	}
	
	public synchronized boolean isConnected(){
		return connectionState == ConnectionState.CONNECTED;
	}
	
	/**
	 * Checks if a the CDAP connection can be opened (i.e. an M_CONNECT message can be sent)
	 * @throws CDAPException
	 */
	public synchronized void checkConnect() throws CDAPException{
		if (!connectionState.equals(ConnectionState.NULL)){
			throw new CDAPException("Cannot open a new connection because " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}
	}
	
	public void connectSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		if (sent){
			connect();
		}else{
			connectReceived(cdapMessage);
		}
	}

	/**
	 * The AE has sent an M_CONNECT message
	 * @throws CDAPException
	 */
	private synchronized void connect() throws CDAPException {
		checkConnect();
		connectionState = ConnectionState.AWAITCON;
		openTimer = new Timer();
		openTimer.schedule(new TimerTask(){
			public void run(){
				log.error("M_CONNECT_R message not received within "+timeout+" ms." +
				"Reseting the connection");
				connectionState = ConnectionState.NULL;
				cdapSession.stopConnection();
			}
		}, timeout);
	}
	
	/**
	 * An M_CONNECT message has been received, update the state
	 * @param message
	 */
	private synchronized void connectReceived(CDAPMessage message) throws CDAPException{
		if (!connectionState.equals(ConnectionState.NULL)){
			throw new CDAPException("Cannot open a new connection because " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}
		connectionState = ConnectionState.AWAITCON;
	}
	
	/**
	 * Checks if the CDAP M_CONNECT_R message can be sent
	 * @throws CDAPException
	 */
	public synchronized void checkConnectResponse() throws CDAPException{
		if (!connectionState.equals(ConnectionState.AWAITCON)){
			throw new CDAPException("Cannot send a connection response because " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}
	}
	
	public synchronized void connectResponseSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		if (sent){
			connectResponse();
		}else{
			connectResponseReceived(cdapMessage);
		}
	}
	
	/**
	 * The AE has sent an M_CONNECT_R  message
	 * @param openConnectionResponseMessage
	 * @throws CDAPException
	 */
	private void connectResponse() throws CDAPException {
		checkConnectResponse();
		connectionState = ConnectionState.CONNECTED;
	}
	
	/**
	 * An M_CONNECT_R message has been received
	 * @param message
	 */
	private void connectResponseReceived(CDAPMessage message) throws CDAPException{
		if (!connectionState.equals(ConnectionState.AWAITCON)){
			throw new CDAPException("Received an M_CONNECT_R message, but " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}

		openTimer.cancel();
		openTimer = null;
		connectionState = ConnectionState.CONNECTED;
	}
	
	/**
	 * Checks if the CDAP M_RELEASE message can be sent
	 * @throws CDAPException
	 */
	public synchronized void checkRelease() throws CDAPException{
		if (!connectionState.equals(ConnectionState.CONNECTED)){
			throw new CDAPException("Cannot close a connection because " +
					"this CDAP session is " +
					"currently in "+connectionState.toString()+" state");
		}
	}
	
	public synchronized void releaseSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		if (sent){
			release(cdapMessage);
		}else{
			releaseReceived(cdapMessage);
		}
	}
	
	/**
	 * The AE has sent an M_RELEASE message
	 * @param releaseConnectionRequestMessage
	 * @throws CDAPException
	 */
	private void release(CDAPMessage cdapMessage) throws CDAPException {
		checkRelease();
		connectionState = ConnectionState.AWAITCLOSE;
		if (cdapMessage.getInvokeID() != 0){
			closeTimer = new Timer();
			closeTimer.schedule(new TimerTask(){
				public void run(){
					log.error("M_RELEASE_R message not received within "+timeout+" ms." +
					"Seting the connection to NULL");
					connectionState = ConnectionState.NULL;
					cdapSession.stopConnection();
				}
			}, timeout);
		}
	}
	
	/**
	 * An M_RELEASE message has been received
	 * @param message
	 */
	private void releaseReceived(CDAPMessage message) throws CDAPException{
		if (!connectionState.equals(ConnectionState.CONNECTED) && !connectionState.equals(ConnectionState.AWAITCLOSE)){
			throw new CDAPException("Cannot close the connection because " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}

		if (message.getInvokeID() != 0
				&& !connectionState.equals(ConnectionState.AWAITCLOSE)){
			connectionState = ConnectionState.AWAITCLOSE;
		}else{
			connectionState = ConnectionState.NULL;
			cdapSession.stopConnection();
		}
	}
	
	/**
	 * Checks if the CDAP M_RELEASE_R message can be sent
	 * @throws CDAPException
	 */
	public synchronized void checkReleaseResponse() throws CDAPException{
		if (!connectionState.equals(ConnectionState.AWAITCLOSE)){
			throw new CDAPException("Cannot send a release connection response message because " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}
	}
	
	public synchronized void releaseResponseSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		if (sent){
			releaseResponse();
		}else{
			releaseResponseReceived();
		}
	}

	/**
	 * The AE has called the close response operation
	 * @param releaseConnectionRequestMessage
	 * @throws CDAPException
	 */
	private void releaseResponse() throws CDAPException{
		checkReleaseResponse();
		connectionState = ConnectionState.NULL;
	}
	
	/**
	 * An M_RELEASE_R message has been received 
	 * @param message
	 */
	private void releaseResponseReceived() throws CDAPException{
		if (!connectionState.equals(ConnectionState.AWAITCLOSE)){
			throw new CDAPException("Received an M_RELEASE_R message, but " +
					"this CDAP session is currently in "+connectionState.toString()+" state");
		}

		closeTimer.cancel();
		closeTimer = null;
		connectionState = ConnectionState.NULL;
		cdapSession.stopConnection();
	}
}
