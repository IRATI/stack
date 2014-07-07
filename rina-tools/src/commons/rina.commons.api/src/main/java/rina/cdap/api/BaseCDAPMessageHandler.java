package rina.cdap.api;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.message.CDAPMessage;
import rina.ribdaemon.api.RIBDaemonException;

/**
 * A basic CDAPMessageHandler that logs as an error all the 
 * messages it receives. Classes extending this class must 
 * overwrite the operations that they are interested in 
 * handling.
 * @author eduardgrasa
 *
 */
public abstract class BaseCDAPMessageHandler implements CDAPMessageHandler{
	
	private static final Log log = LogFactory.getLog(BaseCDAPMessageHandler.class);

	public void createResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void deleteResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void readResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void cancelReadResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void writeResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void startResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}

	public void stopResponse(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		logMessageAsError(cdapMessage, cdapSessionDescriptor);
	}
	
	/**
	 * Log the received CDAP Message as an error
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	private void logMessageAsError(CDAPMessage cdapMessage, 
			CDAPSessionDescriptor cdapSessionDescriptor){
		log.error("Received the following CDAP Message from the flow: "+
				cdapSessionDescriptor.getPortId()+ "\n." + cdapMessage.toString());
	}

}
