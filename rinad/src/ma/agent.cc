#include "agent.h"

//Subsystems
#include "bgtm.h"
#include "confm.h"
#include "flowm.h"
#include "ribf.h"
#include "ribs/ribd_v1.h"

#define RINA_PREFIX "mad"
#include <librina/logs.h>

// std libraries
#include <list>

namespace rinad {
namespace mad {

//Singleton instance
Singleton<ManagementAgent_> ManagementAgent;

//Add NMS DIF
void ManagementAgent_::addNMSDIF(std::string& difName){
	nmsDIFs.push_back(difName);
}

//Creates the NMS DIFs required by the MA
void ManagementAgent_::bootstrapNMSDIFs(){
	//TODO FIXME XXX
	std::list<std::string>::const_iterator it;

	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Bootstraping DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager bootstrapping of the DIFs
	}

}
//Registers the application in the IPCManager
void ManagementAgent_::reg(){

        unsigned int seqnum;
	rina::ApplicationRegistrationInformation ari;
	std::list<std::string>::const_iterator it;
        rina::RegisterApplicationResponseEvent *resp;
	rina::IPCEvent *event;

	//Check if there are available DIFs
	if(nmsDIFs.empty()){
		LOG_ERR("No DIFs to register to. Aborting...");
		throw eNoNMDIF("No DIFs to register to");
	}

	LOG_DBG("Will register as %s, to DIF: %s", info.processName.c_str(),
						(*nmsDIFs.begin()).c_str());
	//Register to DIF0 (FIXME)
	ari.ipcProcessId = 0;  //TODO: remove!
	ari.appName = info;
	ari.applicationRegistrationType =
	rina::APPLICATION_REGISTRATION_SINGLE_DIF;
	ari.difName = rina::ApplicationProcessNamingInformation(
							*nmsDIFs.begin(),
							std::string());

	// Request the registration
	seqnum = rina::ipcManager->requestApplicationRegistration(ari);

	// Wait for the response to come
	while(1){
		event = rina::ipcEventProducer->eventWait();
		if (event && event->eventType ==
			rina::REGISTER_APPLICATION_RESPONSE_EVENT &&
			event->sequenceNumber == seqnum)
			break;
	}

	resp = dynamic_cast<rina::RegisterApplicationResponseEvent*>(event);

	// Update librina state
	if (resp->result == 0){
		rina::ipcManager->commitPendingRegistration(seqnum,
								resp->DIFName);
	}else{
		rina::ipcManager->withdrawPendingRegistration(seqnum);
		LOG_ERR("FATAL ERROR: unable to register to DIF '%s'",
					ari.difName.processName.c_str());
		throw rina::ApplicationRegistrationException("Failed to register application");
	}

	/*
	//TODO: this is for when librina supports multiple SINGLE DIF regs
	//and the config is properly read
	for(it=nmsDIFs.begin(); it!=nmsDIFs.end(); ++it) {
		//Nice trace
		LOG_INFO("Registering agent at DIF '%s'", it->c_str());

		//TODO FIXME XXX: call ipcmanager to register MA to this DIF
	}
	*/
}


//Initialization and destruction routines
void ManagementAgent_::init(const std::string& conf,
					const std::string& cl_logfile,
					const std::string& cl_loglevel){

	try
	{
	//Nice trace
	LOG_INFO("Initializing components...");

	//ConfManager must be initialized first, to
	//proper configure the logging according to the cli level
	//or the config file
	ConfManager->init(conf, cl_logfile, cl_loglevel);

	/*
	* Initialize subsystems
	*/
	//Create RIBs
	//TODO charge from configuration
	std::list<uint64_t> supported_versions;
	RIBFactory->init(supported_versions);

	//TODO
	//FlowManager
	FlowManager->init();


	//Background task manager; MUST be the last one
	//Will not return until SIGINT is sent
	BGTaskManager->init();

	/*
	* Load configuration
	*/
	ConfManager->configure();

	//Bootstrap necessary NMS DIFs and shim-DIFs
	bootstrapNMSDIFs();

	//Register agent AP into the IPCManager
	reg();

	LOG_INFO("Components initialized");

	/*
	* Run the bg task manager loop in the main thread
	*/
	BGTaskManager->run(NULL);
	}
	catch(Exception &e1)
	{
		LOG_ERR("Program finished due to a bad operation");
	}
	FlowManager->runIOLoop();
}

void ManagementAgent_::destroy(){
	/*
	* Destroy all subsystems
	*/

	//TODO

	//FlowManager
	FlowManager->destroy();

	//Bg
	BGTaskManager->destroy();

	//Conf Manager
	ConfManager->destroy();

	LOG_INFO("Goodbye!");
}


//Constructors destructors (singleton only)
ManagementAgent_::ManagementAgent_(){
}


ManagementAgent_::~ManagementAgent_(void){
	LOG_INFO(" Goodbye!");
}

}; //namespace mad
}; //namespace rinad
