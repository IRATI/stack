#include "ribd_v1.h"

#define RINA_PREFIX "mad.ribd_v1"
#include <librina/logs.h>

namespace rinad{
namespace mad{


void ConHandler::connect(int message_id, const cdap_rib::con_handle_t &con)
{
  (void) message_id;
  (void) con;
}
void ConHandler::connectResponse(const cdap_rib::res_info_t &res,
                     const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void ConHandler::release(int message_id, const cdap_rib::con_handle_t &con)
{
  (void) message_id;
  (void) con;
}
void ConHandler::releaseResponse(const cdap_rib::res_info_t &res,
                     const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}

void RespHandler::createResponse(const cdap_rib::res_info_t &res,
                    const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::deleteResponse(const cdap_rib::res_info_t &res,
                    const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::readResponse(const cdap_rib::res_info_t &res,
                  const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::cancelReadResponse(const cdap_rib::res_info_t &res,
                        const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::writeResponse(const cdap_rib::res_info_t &res,
                   const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::startResponse(const cdap_rib::res_info_t &res,
                   const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}
void RespHandler::stopResponse(const cdap_rib::res_info_t &res,
                  const cdap_rib::con_handle_t &con)
{
  (void) res;
  (void) con;
}

// CLASS RIBDaemonv1
RIBDaemonv1_::RIBDaemonv1_()
{
  resp_handler_ =  new RespHandler();
  conn_handler_ = new ConHandler();
}
RIBDaemonv1_::~RIBDaemonv1_()
{
  delete resp_handler_;
  delete conn_handler_;
}

const rib::ResponseHandlerInterface* RIBDaemonv1_::getRespHandler()
{
  return resp_handler_;
}
const cacep::AppConHandlerInterface* RIBDaemonv1_::getConnHandler()
{
  return conn_handler_;
}
void RIBDaemonv1_::initiateRIB(rib::RIBDInterface* ribd)
{
	try
	{
	  rib::EmptyEncoder *enc = new rib::EmptyEncoder();
	  ribd->addRIBObject(new rib::EmptyRIBObject("ROOT", "root", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("DAF", "root, dafID=1", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("ComputingSystem", "root, computingSystemID=1", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("ProcessingSystem", "root, computingSystemID = 1, processingSystemID=1", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("Software", "root, computingSystemID = 1, processingSystemID=1, software", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("Hardware", "root, computingSystemID = 1, processingSystemID=1, hardware", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("KernelApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("OSApplicationProcess", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("ManagementAgent", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID=1", intance_gen_.get_id(), enc));
		// IPCManagement branch
	  ribd->addRIBObject(new rib::EmptyRIBObject("IPCManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("IPCResourceManager", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("UnderlayingFlows", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingFlows", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("UnderlayingDIFs", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingDIFs", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("QueryDIFAllocator", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, queryDIFAllocator", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("UnderlayingRegistrations", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"ipcResourceManager, underlayingRegistrations", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("SDUPRotection", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ipcManagement, "
				"sduProtection", intance_gen_.get_id(), enc));
		// RIBDaemon branch
	  ribd->addRIBObject(new rib::EmptyRIBObject("RIBDaemon", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon", intance_gen_.get_id(), enc));
	  ribd->addRIBObject(new rib::EmptyRIBObject("Discriminators", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, ribDaemon"
				", discriminators", intance_gen_.get_id(), enc));
		// DIFManagement
	  ribd->addRIBObject(new rib::EmptyRIBObject("DIFManagement", "root, computingSystemID = 1, "
				"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, magementAgentID = 1, difManagement", intance_gen_.get_id(), enc));
	}
	catch(Exception &e1)
	{
		LOG_ERR("RIB basic objects were not created because %s", e1.what());
		throw Exception("Finish application");
	}
}


}; //namespace mad
}; //namespace rinad


