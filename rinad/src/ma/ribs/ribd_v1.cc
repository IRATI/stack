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
		addRIBObject(new SimplestRIBObj(this, "ROOT", "root"));
		addRIBObject(new SimpleRIBObj(this, "DAF", "root, dafID", 1));
		addRIBObject(new SimpleRIBObj(this, "ComputingSystem", "root, computingSystemID", 1));
		addRIBObject(new SimpleRIBObj(this, "ProcessingSystem", "root, computingSystemID = 1, processingSystemID", 1));
		addRIBObject(new SimplestRIBObj(this, "Software", "root, computingSystemID = 1, processingSystemID=1, software"));
		addRIBObject(new SimplestRIBObj(this, "Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware"));
		addRIBObject(new SimplestRIBObj(this, "KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess"));
		addRIBObject(new SimplestRIBObj(this, "OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess"));
		addRIBObject(new SimpleRIBObj(this, "ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID", 1));
		// IPCManagement branch
		addRIBObject(new SimplestRIBObj(this, "IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement"));
		addRIBObject(new SimplestRIBObj(this, "IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager"));
		addRIBObject(new SimplestRIBObj(this, "UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows"));
		addRIBObject(new SimplestRIBObj(this, "UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs"));
		addRIBObject(new SimplestRIBObj(this, "QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator"));
		addRIBObject(new SimplestRIBObj(this, "UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations"));
		addRIBObject(new SimplestRIBObj(this, "SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection"));
		// RIBDaemon branch
		addRIBObject(new SimplestRIBObj(this, "RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"));
		addRIBObject(new SimplestRIBObj(this, "Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators"));
		// DIFManagement
		addRIBObject(new SimplestRIBObj(this, "DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement"));
	}
	catch(Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw Exception("Finish application");
	}
}

void RIBDaemonv1::sendMessageSpecific(bool useAddress,
			const rina::CDAPMessage& cdapMessage,
			int sessionId,
                        unsigned int address,
			rina::ICDAPResponseMessageHandler* cdapMessageHandler){

	//XXX FIXME: fill-in
	(void)useAddress;
	(void)cdapMessage;
	(void)sessionId;
	(void)address;
	(void)cdapMessageHandler;
}


//CLASS SimplestRIBObj
SimplestRIBObj::SimplestRIBObj(rina::IRIBDaemon *rib_daemon,	const std::string& object_class, const std::string& object_name):
		rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{}
const void* SimplestRIBObj::get_value() const
{
	return 0;
}

//CLASS SimpleRIBObj
SimpleRIBObj::SimpleRIBObj(rina::IRIBDaemon *rib_daemon,	const std::string& object_class, const std::string& object_name, unsigned int id):
		rina::BaseRIBObject(rib_daemon, object_class, rina::objectInstanceGenerator->getObjectInstance(), object_name)
{
	id_ = id;
}
const void* SimpleRIBObj::get_value() const
{
	return 0;
}

}; //namespace mad
}; //namespace rinad


