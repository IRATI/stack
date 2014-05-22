package rina.ipcprocess.impl.pduftg.linkstate.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.pduftg.linkstate.LinkStatePDUFTGeneratorPolicyImpl;

public class ComputePDUFT extends TimerTask{
	
	LinkStatePDUFTGeneratorPolicyImpl pduFT = null;

	public ComputePDUFT(LinkStatePDUFTGeneratorPolicyImpl pduFT)
	{
		this.pduFT = pduFT;
	}
	@Override
	public void run() 
	{
		this.pduFT.forwardingTableUpdate();
	}

}
