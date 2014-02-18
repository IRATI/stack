package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.TimerTask;

import rina.PDUForwardingTable.api.PDUFTable;

public class PropagateFSODB extends TimerTask{
	
	PDUFTable pduFT = null;
	
	public PropagateFSODB(PDUFTable pduFT)
	{
		this.pduFT = pduFT;
	}
	
	@Override
	public void run() {
		this.pduFT.propagateFSDB();
	}
	

}
