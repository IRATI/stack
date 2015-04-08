/**
* @file ribdv1.h
* @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
*
* @brief Management Agent RIB daemon v1
*/


#ifndef __RINAD_IPCP_OBJ_H__
#define __RINAD_IPCP_OBJ_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

namespace rinad{
namespace mad{
namespace rib_v1{


//fwd decl
class IPCPObj;

/**
* Encoder
*/
class IPCPEncoder : public rina::rib::Encoder<IPCPObj>{
  virtual void encode(const IPCPObj &obj,
                              rina::cdap_rib::SerializedObject& serobj) const;
  virtual void decode(const rina::cdap_rib::SerializedObject &serobj,
                                            IPCPObj& des_obj) const;

  virtual std::string get_type() const{ return "ipcp"; };
};

/**
* IPCP object
*/
class IPCPObj : public rina::rib::RIBObject<IPCPObj>{

public:
  IPCPObj(std::string name, long instance, int ipcp_id);
  virtual ~IPCPObj(){};

  //We only support deletion
  virtual bool deleteObject(const void* value);

  //Name of the class
  const static std::string class_name;

private:
  int processID_;
  IPCPEncoder encoder;
};


}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_IPCP_OBJ_H__ */
