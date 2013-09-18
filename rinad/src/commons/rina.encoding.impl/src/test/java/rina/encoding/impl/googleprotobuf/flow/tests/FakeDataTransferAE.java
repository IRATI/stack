package rina.encoding.impl.googleprotobuf.flow.tests;

import rina.aux.BlockingQueueWithSubscriptor;
import rina.efcp.api.BaseDataTransferAE;
import rina.efcp.api.PDUParser;
import rina.flowallocator.api.Flow;
import rina.ipcservice.api.APService;
import rina.ipcservice.api.IPCException;

public class FakeDataTransferAE extends BaseDataTransferAE{

	@Override
	public void createConnectionAndBindToPortId(Flow arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void freeCEPIds(int arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public int[] reserveCEPIds(int arg0, int arg1) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void createLocalConnectionAndBindToPortId(int arg0, int arg1, APService applicationCallback) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void subscribeToFlow(int arg0) throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public BlockingQueueWithSubscriptor getIncomingConnectionQueue(long arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public BlockingQueueWithSubscriptor getOutgoingConnectionQueue(long arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		return null;
	}



	@Override
	public void deleteConnection(long arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public PDUParser getPDUParser() {
		// TODO Auto-generated method stub
		return null;
	}

}
