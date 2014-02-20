package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.TimerTask;

import rina.PDUForwardingTable.api.PDUFTable;

public class UpdateAge extends TimerTask{

	PDUFTable pduFT = null;

	public UpdateAge(PDUFTable pduFT)
	{
		this.pduFT = pduFT;
	}
	@Override
	public void run() 
	{
		this.pduFT.updateAge();
	}
}
