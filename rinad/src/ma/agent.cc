#include "agent.h"

//Subsystems
#include "bgtm.h"
#include "confm.h"
#include "flowm.h"
#include "rib-objects.h"

#define RINA_PREFIX "mad"
#include <librina/logs.h>

namespace rinad {
namespace mad {

//Static initializations
ManagementAgent* ManagementAgent::inst = NULL;

void ManagementAgent::init(const std::string& conf, const std::string& logfile,
						const std::string& loglevel){
	if(inst){
		throw Exception(
			"Invalid double call to ManagementAgent::init()");
	}
	inst = new ManagementAgent(conf, logfile, loglevel);
}

void ManagementAgent::destroy(void){
	//TODO: be gentle with on going events
	delete ManagementAgent::inst;
}

//Constructors destructors
ManagementAgent::ManagementAgent(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){

	try
	{
	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	ConfManager::init(conf, cl_logfile, cl_loglevel);

	//Nice trace
	LOG_INFO(" Initializing...");

	/*
	* Initialize subsystems
	*/
	//RIB & RIBDaemon
	rib_daemon_ = new MARIBDaemon(0);
	populateBasicRIB();

	//TODO
	//FlowManager
	FlowManager::init();

	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	BGTaskManager::init();

	/*
	* Load configuration
	*/
	ConfManager::get()->configure();

	/*
	* Run the bg task manager loop in the main thread
	*/
	BGTaskManager::get()->run(NULL);
	}
	catch(Exception &e1)
	{
		LOG_ERR("Program finished due to a bad operation");
	}
}

ManagementAgent::~ManagementAgent(void){

	/*
	* Destroy all subsystems
	*/

	//TODO

	//FlowManager
	FlowManager::destroy();

	//Bg
	BGTaskManager::destroy();

	//
	delete rib_daemon_;

	LOG_INFO(" Goodbye!");
}

void ManagementAgent::populateBasicRIB(){
	try
	{
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "ROOT", "root"));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "DAF", "root, dafID", 1));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ComputingSystem", "root, computingSystemID", 1));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ProcessingSystem", "root, computingSystemID = 1, processingSystemID", 1));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Software", "root, computingSystemID = 1, processingSystemID=1, software"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess"));
		rib_daemon_->addRIBObject(new SimpleRIBObj(rib_daemon_, "ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID", 1));
		// IPCManagement branch
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection"));
		// RIBDaemon branch
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"));
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators"));
		// DIFManagement
		rib_daemon_->addRIBObject(new SimplestRIBObj(rib_daemon_, "DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement"));
	}
	catch(Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw Exception("Finish application");
	}
}
}; //namespace mad
}; //namespace rinad
