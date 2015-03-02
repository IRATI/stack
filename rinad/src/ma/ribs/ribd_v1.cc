#include "ribd_v1.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define RINA_PREFIX "mad.ribd_v1"
#include <librina/logs.h>

namespace rinad{
namespace mad{

//TODO: change these const
const unsigned int COMPUTING_SYSTEM_ID = 1;

// CLASS RIBDaemonv1
RIBDaemonv1::RIBDaemonv1 (const rina::RIBSchema *schema): rina::RIBDaemon(schema)
{
	try
	{
		addRIBObject(new SimpleRIBObj(this, "ROOT", "root"));
		addRIBObject(new SimpleRIBObj(this, "DAF", "root, dafID=1"));
		addRIBObject(new SimpleRIBObj(this, "ComputingSystem", "root, computingSystemID=1"));
		addRIBObject(new SimpleRIBObj(this, "ProcessingSystem", "root, computingSystemID = 1, processingSystemID=1"));
		addRIBObject(new SimpleRIBObj(this, "Software", "root, computingSystemID = 1, processingSystemID=1, software"));
		addRIBObject(new SimpleRIBObj(this, "Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware"));
		addRIBObject(new SimpleRIBObj(this, "KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess"));
		addRIBObject(new SimpleRIBObj(this, "OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess"));
		addRIBObject(new SimpleRIBObj(this, "ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID=1"));
		// IPCManagement branch
		addRIBObject(new SimpleRIBObj(this, "IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement"));
		addRIBObject(new SimpleRIBObj(this, "IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager"));
		addRIBObject(new SimpleRIBObj(this, "UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows"));
		addRIBObject(new SimpleRIBObj(this, "UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs"));
		addRIBObject(new SimpleRIBObj(this, "QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator"));
		addRIBObject(new SimpleRIBObj(this, "UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations"));
		addRIBObject(new SimpleRIBObj(this, "SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection"));
		// RIBDaemon branch
		addRIBObject(new SimpleRIBObj(this, "RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"));
		addRIBObject(new SimpleRIBObj(this, "Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators"));
		// DIFManagement
		addRIBObject(new SimpleRIBObj(this, "DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement"));
	}
	catch(Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw Exception("Finish application");
	}
}

void RIBDaemonv1::sendMessageSpecific(const rina::RemoteProcessId &remote_proc,
		const rina::CDAPMessage & cdapMessage,
		rina::ICDAPResponseMessageHandler *cdapMessageHandler){

	//XXX FIXME: fill-in
	(void)remote_proc.use_address_;
	(void)cdapMessage;
	(void)remote_proc.port_id_;
	(void)remote_proc.address_;
	(void)cdapMessageHandler;
}


//CLASS SimpleRIBObj
SimpleRIBObj::SimpleRIBObj(rina::IRIBDaemon *rib_daemon,	const std::string& object_class, const std::string& object_name):
		rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{}

const void* SimpleRIBObj::get_value() const
{
	return 0;
}

}; //namespace mad
}; //namespace rinad


