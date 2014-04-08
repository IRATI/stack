package rina.ipcprocess.impl.PDUForwardingTable;

import rina.PDUForwardingTable.api.PDUFTable;
import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ribdaemon.api.RIBDaemonException;

public class PDUFTCDAPMessageHandler implements CDAPMessageHandler {
	protected PDUFTable pdufTable = null;
	
	PDUFTCDAPMessageHandler(PDUFTable pdufTable)
	{
		this.pdufTable = pdufTable;
	}
	@Override
	public void cancelReadResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

	@Override
	public void createResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

	@Override
	public void deleteResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

	@Override
	public void readResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException 
	{
		pdufTable.writeMessageRecieved(cdapMessage, cdapSessionDescriptor.getPortId());
	}

	@Override
	public void startResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

	@Override
	public void stopResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

	@Override
	public void writeResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
	}

}
