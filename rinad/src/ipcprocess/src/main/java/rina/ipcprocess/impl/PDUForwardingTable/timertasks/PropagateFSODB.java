package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.PDUForwardingTable.PDUFTInt;

public class PropagateFSODB extends TimerTask{
	
	PDUFTInt pduFT = null;
	
	public PropagateFSODB(PDUFTInt pduFT)
	{
		this.pduFT = pduFT;
	}
	
	@Override
	public void run() {
		this.pduFT.propagateFSDB();
	}
	

}
