package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.PDUForwardingTable.PDUFTInt;

public class UpdateAge extends TimerTask{

	PDUFTInt pduFT = null;

	public UpdateAge(PDUFTInt pduFT)
	{
		this.pduFT = pduFT;
	}
	@Override
	public void run() 
	{
		this.pduFT.updateAge();
	}
}
