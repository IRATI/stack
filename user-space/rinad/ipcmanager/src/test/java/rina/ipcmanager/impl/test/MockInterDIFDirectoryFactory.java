package rina.ipcmanager.impl.test;

import rina.idd.api.InterDIFDirectory;
import rina.idd.api.InterDIFDirectoryFactory;
import rina.ipcmanager.api.IPCManager;

public class MockInterDIFDirectoryFactory implements InterDIFDirectoryFactory{

	public InterDIFDirectory createIDD(IPCManager ipcManager) {
		return new MockInterDIFDirectory();
	}

}
