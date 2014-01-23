package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.PDUForwardingTable.PDUFTInt;

public class ComputePDUFT extends TimerTask{
	
	PDUFTInt pduFT = null;

	public ComputePDUFT(PDUFTInt pduFT)
	{
		this.pduFT = pduFT;
	}
	@Override
	public void run() 
	{
		this.pduFT.ForwardingTableupdate();
	}

}
