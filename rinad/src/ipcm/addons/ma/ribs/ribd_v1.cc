/*
 * RIB factory
 *
 *    Bernat Gaston         <bernat.gaston@i2cat.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ribd_v1.h"

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>
#include "../../../ipcm.h"

//Object definitions
#include "ipcp_obj.h"
#include "os_proc_obj.h"
#include "ribd_obj.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Instance generator
Singleton<rina::ConsecutiveUnsignedIntegerGenerator> inst_gen;

void RIBConHandler_v1::connect(int message_id,
				const rina::cdap_rib::con_handle_t &con) {
	(void) message_id;
	(void) con;
}

void RIBConHandler_v1::connectResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}
void RIBConHandler_v1::release(int message_id,
				const rina::cdap_rib::con_handle_t &con) {
	(void) message_id;
	(void) con;
}
void RIBConHandler_v1::releaseResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}

void RIBRespHandler_v1::createResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler_v1::deleteResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}
void RIBRespHandler_v1::readResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler_v1::cancelReadResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
}
void RIBRespHandler_v1::writeResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler_v1::startResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
	(void) obj;
}
void RIBRespHandler_v1::stopResponse(
		const rina::cdap_rib::res_info_t &res,
		const rina::cdap_rib::obj_info_t &obj,
		const rina::cdap_rib::con_handle_t &con) {
	(void) res;
	(void) con;
	(void) obj;
}

// Initializes the RIB with the current status
void initiateRIB(rina::rib::RIBDNorthInterface* ribd) {
	try {
		rina::rib::EmptyEncoder *enc = new rina::rib::EmptyEncoder();
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"ROOT", "root",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"DAF", "root, dafID=1",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"ComputingSystem",
						"root, computingSystemID=1",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"ProcessingSystem",
						"root, computingSystemID = 1, processingSystemID=1",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"Software",
						"root, computingSystemID = 1, processingSystemID=1, software",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"Hardware",
						"root, computingSystemID = 1, processingSystemID=1, hardware",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"KernelApplicationProcess",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new OSApplicationProcessObj(
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess",
						inst_gen->next(), ribd));
		ribd->addRIBObject(
				new OSApplicationProcessObj(
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses",
						inst_gen->next(), ribd));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"ManagementAgent",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID=1",
						inst_gen->next(), enc));
		// IPCManagement branch
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"IPCManagement",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"IPCResourceManager",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"ipcResourceManager",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"UnderlayingFlows",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"ipcResourceManager, underlayingFlows",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"UnderlayingDIFs",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"ipcResourceManager, underlayingDIFs",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"QueryDIFAllocator",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"ipcResourceManager, queryDIFAllocator",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"UnderlayingRegistrations",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"ipcResourceManager, underlayingRegistrations",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"SDUPRotection",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ipcManagement, "
						"sduProtection",
						inst_gen->next(), enc));
		// RIBDaemon branch
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"RIBDaemon",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ribDaemon",
						inst_gen->next(), enc));
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"Discriminators",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, ribDaemon"
						", discriminators",
						inst_gen->next(), enc));
		// DIFManagement
		ribd->addRIBObject(
				new rina::rib::EmptyRIBObject(
						"DIFManagement",
						"root, computingSystemID = 1, "
						"processingSystemID=1, kernelApplicationProcess, osApplicationProcess, managementAgentID = 1, difManagement",
						inst_gen->next(), enc));

		//
		//Add the IPCPs
		//
		std::list<int> ipcps;
		IPCManager->list_ipcps(ipcps);

		std::list<int>::const_iterator it;
		for (it = ipcps.begin(); it != ipcps.end(); ++it) {

			//FIXME: this should be simplified. Object constructors
			//should contain inner objects

			//Add the IPCP and add the RIBDaemon
			std::stringstream ss;
			ss << "root, computingSystemID = 1, processingSystemID = 1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ";
			ss << "ipcProcessID = " << (*it);
			ribd->addRIBObject(
					new IPCPObj(ss.str(), inst_gen->next(),
							(*it)));

			ss << ", ribdaemon";
			ribd->addRIBObject(
					new RIBDaemonObj(
							ss.str(),
							inst_gen->next(),
							(*it)));
		}

	} catch (rina::Exception &e1) {
		LOG_ERR("RIB basic objects were not created because %s",
			e1.what());
		throw rina::Exception("Finish application");
	}
}

void createIPCPObject(rina::rib::RIBDNorthInterface &ribd, int ipcp_id) {
	IPCPObj* ipcp;

	std::stringstream ss;
	ss << "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=";
	ss << ipcp_id;
	try {
		ipcp = new IPCPObj(ss.str(), inst_gen->next(), ipcp_id);

		ribd.addRIBObject(ipcp);
	} catch (...) {
		LOG_ERR("Unable to create an IPCP object '%s'; out of memory?",
			ss.str().c_str());
	}
}

void destroyIPCPObject(rina::rib::RIBDNorthInterface &ribd, int ipcp_id) {
	std::stringstream ss;
	ss << "root, computingSystemID = 1, processingSystemID=1, kernelApplicationProcess, osApplicationProcess, ipcProcesses, ipcProcessID=";
	ss << ipcp_id;
	try {
		ribd.removeRIBObject(ss.str());
	} catch (...) {
		LOG_ERR("Unable to delete an IPCP object '%s'",
			ss.str().c_str());
	}
}

}//namespace rib_v1;
}//namespace mad
}//namespace rinad

