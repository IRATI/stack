#include <ipcp_obj.h>

#define RINA_PREFIX "ipcm.mad.ribd_v1"
#include <librina/logs.h>
#include <librina/exceptions.h>

//Include the GPB
#include "common/encoders/MA-IPCP.pb.h"

#include "../../../ipcm.h"

namespace rinad{
namespace mad{
namespace rib_v1{

const std::string IPCPObj::class_name = "IPCProcess";


//Encoder
void IPCPEncoder::encode(const IPCPObj &obj,
                               rina::cdap_rib::SerializedObject& serobj) const{

  (void)serobj;
  (void)obj;
}

void IPCPEncoder::decode(const rina::cdap_rib::SerializedObject &serobj,
                                            IPCPObj& des_obj) const{

  (void)serobj;
  (void)des_obj;
}


//Class

IPCPObj::IPCPObj(std::string name, long instance, int ipcp_id):
             RIBObject<IPCPObj>(class_name, instance, name, this, &encoder),
             processID_(ipcp_id){

}

//We only support deletion
bool IPCPObj::deleteObject(const void* value){

  (void)value;

  //Call the IPCManager and return
	if(IPCManager->destroy_ipcp(processID_) != IPCM_SUCCESS){
		LOG_ERR("Unable to destroy IPCP with id %d", processID_);
    return false;
  }

  return true;
}

}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad
