#include "ipcp_obj.h"

#define RINA_PREFIX "ipcm.mad.ipcp_obj"
#include <cstddef>

#include <librina/logs.h>
#include <librina/exceptions.h>

#include "../agent.h"
#include "../../../ipcm.h"
#include "../ribf.h"

namespace rinad {
namespace mad {
namespace rib_v1 {

//Static class names
const std::string IPCPObj::class_name = "IPCProcess";

//Class
IPCPObj::IPCPObj(int ipcp_id)
                : DelegationObj(class_name)
{
        processID_ = ipcp_id;
}

void IPCPObj::read(const rina::cdap_rib::con_handle_t &con,
                   const std::string& fqn, const std::string& class_,
                   const rina::cdap_rib::filt_info_t &filt, const int invoke_id,
                   rina::ser_obj_t &obj_reply, rina::cdap_rib::res_info_t& res)
{

        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
        configs::ipcp_t info;
        info.process.processInstance = processID_;
        info.process.processName = IPCManager->get_ipcp_name(processID_);
        //TODO: Add missing stuff...
        encoders::IPCPEncoder encoder;
        encoder.encode(info, obj_reply);
}

bool IPCPObj::delete_(const rina::cdap_rib::con_handle_t &con,
                      const std::string& fqn, const std::string& class_,
                      const rina::cdap_rib::filt_info_t &filt,
                      const int invoke_id, rina::cdap_rib::res_info_t& res)
{

        //Fill in the response
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;

        //Call the IPCManager and return
        if (IPCManager->destroy_ipcp(ManagementAgent::inst, processID_)
                        != IPCM_SUCCESS)
        {
                LOG_ERR("Unable to destroy IPCP with id %d", processID_);
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                return false;
        }

        return true;
}

void IPCPObj::forward_object(const rina::cdap_rib::con_handle_t& con,
			     rina::cdap::cdap_m_t::Opcode op_code,
			     const std::string obj_name,
			     const std::string obj_class,
			     const rina::ser_obj_t &obj_value,
			     const rina::cdap_rib::flags_t &flags,
			     const rina::cdap_rib::filt_info_t &filt,
			     const int invoke_id)
{
       // TODO: This has to be stored in a list in case of more than
       // one consecutive request

       std::size_t pos = obj_name.rfind("ipcpid");
       if (pos != std::string::npos)
       {
                std::string object_sub_name = obj_name.substr(pos);
                pos = object_sub_name.find("/");
                if (pos != std::string::npos)
                {
                   object_sub_name = object_sub_name.substr(pos);
            	   // mark processing delegation
            	   activate_delegation();
                   ipcm_res_t res = IPCManager->delegate_ipcp_ribobj(this,
                		   	   	   	   	     processID_,
								     op_code,
								     obj_class,
								     object_sub_name,
								     obj_value,
								     filt.scope_,
								     invoke_id,
								     con.port_id);
                   if (res == IPCM_FAILURE)
                	   signal_finished();
                }
       }
       else
    	   LOG_ERR("This object is not an IPC process");
}

void IPCPObj::create_cb(const rina::rib::rib_handle_t rib,
                        const rina::cdap_rib::con_handle_t &con,
                        const std::string& fqn, const std::string& class_,
                        const rina::cdap_rib::filt_info_t &filt,
                        const int invoke_id, const rina::ser_obj_t &obj_req,
                        rina::cdap_rib::obj_info_t &obj_reply,
                        rina::cdap_rib::res_info_t& res)
{

        IPCPObj* ipcp;

        (void) con;
        (void) filt;
        (void) invoke_id;
        (void) obj_reply;

        //Set return value
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;

        if (class_ != IPCPObj::class_name)
        {
                LOG_ERR("Create operation failed: received an invalid class "
                		"name '%s' during create operation in '%s'",
                        class_.c_str(), fqn.c_str());
                res.code_ = rina::cdap_rib::CDAP_INVALID_OBJ_CLASS;
                return;
        }

        configs::ipcp_config_t object;
        // TODO do the decoders/encoders
        encoders::IPCPConfigEncoder encoder;
        encoder.decode(obj_req, object);

        //Call the IPCManager
        int ipcp_id = createIPCP(object);

        if (ipcp_id == -1)
        {
                std::stringstream ss;
                ss << "Create operation failed in '" << fqn.c_str() << "': unable to create an IPCP.";
                LOG_ERR("%s", ss.str().c_str());
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                res.reason_ = ss.str();
                return;
        }

        if (!assignToDIF(object, ipcp_id))
        {
                std::stringstream ss;
                ss << "Create operation failed for IPCP with id '" << ipcp_id
                   << "' in path '" << fqn.c_str() << "':unable to assign IPCP.";
                LOG_ERR("%s", ss.str().c_str());
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                res.reason_ = ss.str();
                return;
        }

        if (!object.difs_to_register.empty())
        {
                if (!registerAtDIFs(object, ipcp_id))
                {
                	std::stringstream ss;
                        ss << "Unable to fully register IPCP with id '" << ipcp_id
                           << "' in path '" << fqn.c_str() << "'.";
                        LOG_ERR("%s", ss.str().c_str());
                        res.code_ = rina::cdap_rib::CDAP_ERROR;
                        res.reason_ = ss.str();                          
                }
        }

        if (!object.neighbors.empty())
        {
        	enrollToDIFs(object, ipcp_id);
        }

        try
        {
                ipcp = new IPCPObj(ipcp_id);
        } catch (...)
        {
                std::stringstream ss;
                ss << "Create operation failed for IPCP with id '" << ipcp_id
                   << "' in path '" << fqn.c_str() << "': creating RO - out of memory.";
                LOG_ERR("%s", ss.str().c_str());
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                res.reason_ = ss.str();                          
                return;
        }

        //Finally add it into the RIB with the proper IPCP id in the object name
        try
        {
        	std::stringstream ss;
        	ss << fqn << "=" << ipcp_id;
                RIBFactory::getProxy()->addObjRIB(rib, ss.str(), &ipcp);
          	obj_reply.name_ = ss.str();
          	obj_reply.inst_ = ipcp_id;
        } catch (...)
        {
                std::stringstream ss;
                ss << "Create operation failed for IPCP with id '" << ipcp_id
                   << "' in path '" << fqn.c_str() << "': registering RO.";
                LOG_ERR("%s", ss.str().c_str());
                res.code_ = rina::cdap_rib::CDAP_ERROR;
                res.reason_ = ss.str();                          
                return;
        }
        res.code_ = rina::cdap_rib::CDAP_SUCCESS;
}

const std::string IPCPObj::get_displayable_value() const
{
	std::stringstream ss;

	ss << "IPC Process id: " << processID_;

	return ss.str();
}

int IPCPObj::createIPCP(configs::ipcp_config_t &object)
{

        CreateIPCPPromise ipcp_promise;

        rina::ApplicationProcessNamingInformation ipcp_name(
                        object.name.processName, object.name.processInstance);

        if (IPCManager->create_ipcp(ManagementAgent::inst, &ipcp_promise,
                                    ipcp_name, object.dif_to_assign.dif_type_,
				    object.dif_to_assign.dif_name_.processName)
                        == IPCM_FAILURE || ipcp_promise.wait() != IPCM_SUCCESS)
        {
                LOG_ERR("Error while creating IPC process");
                return -1;
        }
        LOG_INFO("IPC process created successfully [id = %d, name= %s]",
                 ipcp_promise.ipcp_id, object.name.processName.c_str());
        return ipcp_promise.ipcp_id;
}

bool IPCPObj::assignToDIF(configs::ipcp_config_t &object, int ipcp_id)
{
        // ASSIGN TO DIF
        Promise assign_promise;
        bool found;

        if (!IPCManager->ipcp_exists(ipcp_id))
        {
                LOG_ERR("No such IPC process id %d", ipcp_id);
                return false;
        }

        if (IPCManager->assign_to_dif(ManagementAgent::inst, &assign_promise,
                                      ipcp_id, object.dif_to_assign,
                                      object.name) == IPCM_FAILURE
                        || assign_promise.wait() != IPCM_SUCCESS)
        {
                LOG_ERR("DIF assignment failed");
                return false;
        }

        LOG_INFO("DIF assignment completed successfully");
        return true;
}

bool IPCPObj::registerAtDIFs(configs::ipcp_config_t &object, int ipcp_id)
{
        for (std::list<rina::ApplicationProcessNamingInformation>::iterator it =
                        object.difs_to_register.begin();
                        it != object.difs_to_register.end(); ++it)
        {
                Promise promise;
                LOG_DBG("Registring to DIF %s", it->processName.c_str());

                if (!IPCManager->ipcp_exists(ipcp_id))
                {
                        LOG_ERR("No such IPC process id");
                        return false;
                }
                if (IPCManager->register_at_dif(ManagementAgent::inst, &promise,
                                                ipcp_id, *it) == IPCM_FAILURE
                                || promise.wait() != IPCM_SUCCESS)
                {
                        LOG_ERR("Registration failed");
                        return false;
                }

                LOG_INFO("IPC process registration to dif %s completed successfully",
                         it->processName.c_str());
        }

        return true;
}

bool IPCPObj::enrollToDIFs(rinad::configs::ipcp_config_t &object, int ipcp_id)
{
	for(std::list<configs::neighbor_config_t>::iterator it =
			object.neighbors.begin();
			it != object.neighbors.end(); ++it)
	{
		Promise promise;
		rinad::NeighborData neighbor;
		LOG_DBG("Enrolling to neighbor %s", it->neighbor_name.processName.c_str());

                if (!IPCManager->ipcp_exists(ipcp_id))
                {
                        LOG_ERR("No such IPC process id");
                        return false;
                }

                neighbor.apName.processName = it->neighbor_name.processName;
                neighbor.apName.processInstance = "1";
                neighbor.difName.processName = it->dif.processName;
                neighbor.supportingDifName.processName = it->under_dif.processName;

                if (IPCManager->enroll_to_dif(ManagementAgent::inst,
                			      &promise, ipcp_id,
					      neighbor) == IPCM_FAILURE
                		|| promise.wait() != IPCM_SUCCESS)
                {
                	LOG_ERR("Enrollment failed");
                	continue;
                }

                LOG_INFO("IPC Process enrollment to neighbor %s completed successfully",
                	 it->neighbor_name.processName.c_str());
	}

	return true;
}

}
;
//namespace rib_v1
}
;
//namespace mad
}
;
//namespace rinad
