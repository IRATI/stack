package rina.ipcprocess.impl.pduftg.linkstate.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.pduftg.linkstate.LinkStatePDUFTGeneratorPolicyImpl;

public class PropagateFSODB extends TimerTask{
	
	LinkStatePDUFTGeneratorPolicyImpl pduFT = null;
	
	public PropagateFSODB(LinkStatePDUFTGeneratorPolicyImpl pduFT)
	{
		this.pduFT = pduFT;
	}
	
	@Override
	public void run() {
		this.pduFT.propagateFSDB();
	}
	

}
