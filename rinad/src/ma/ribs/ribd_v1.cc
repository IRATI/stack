#include "ribd_v1.h"

#define RINA_PREFIX "mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

namespace rinad{
namespace mad{
namespace rib_v1{

Singleton<rina::ConsecutiveUnsignedIntegerGenerator> InstanceGenerator;

void RIBConHandler_v1::connect(int message_id, const rina::cdap_rib::con_handle_t &con)
{
  (void) message_id;
  (void) con;
}
void RIBConHandler_v1::connectResponse(const rina::cdap_rib::res_info_t &res,
                     const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBConHandler_v1::release(int message_id, const rina::cdap_rib::con_handle_t &con)
{
  (void) message_id;
  (void) con;
}
void RIBConHandler_v1::releaseResponse(const rina::cdap_rib::res_info_t &res,
                     const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}

void RIBRespHandler_v1::createResponse(const rina::cdap_rib::res_info_t &res,
                    const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::deleteResponse(const rina::cdap_rib::res_info_t &res,
                    const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::readResponse(const rina::cdap_rib::res_info_t &res,
                  const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::cancelReadResponse(const rina::cdap_rib::res_info_t &res,
                        const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::writeResponse(const rina::cdap_rib::res_info_t &res,
                   const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::startResponse(const rina::cdap_rib::res_info_t &res,
                   const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RIBRespHandler_v1::stopResponse(const rina::cdap_rib::res_info_t &res,
                  const rina::cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void initiateRIB(rina::rib::RIBDNorthInterface* ribd)
{
	try
	{
	  rina::rib::EmptyEncoder *enc = new rina::rib::EmptyEncoder();
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("ROOT", "root", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("DAF", "root, dafID=1", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("ComputingSystem", "root, computingSystemID=1", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("ProcessingSystem", "root, computingSystemID = 1, processingSystemID=1", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("Software", "root, computingSystemID = 1, processingSystemID=1, software", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID=1", InstanceGenerator->next(), enc));
		// IPCManagement branch
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection", InstanceGenerator->next(), enc));
		// RIBDaemon branch
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon", InstanceGenerator->next(), enc));
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators", InstanceGenerator->next(), enc));
		// DIFManagement
	  ribd->addRIBObject(new rina::rib::EmptyRIBObject("DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement", InstanceGenerator->next(), enc));
	}
	catch(rina::Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw rina::Exception("Finish application");
	}
}

}; //namespace rib_v1;
}; //namespace mad
}; //namespace rinad


