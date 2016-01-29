/**
 * @file ribdv1.h
 * @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
 *
 * @brief Management Agent RIB daemon v1
 */

#ifndef __RINAD_RIBD_V1_H__
#define __RINAD_RIBD_V1_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

namespace rinad {
namespace mad {
namespace rib_v1 {

///
/// Create v1 schema
///
void createSchema(void);

///
/// Create and initialize a RIB (populate static objects)
///
rina::rib::rib_handle_t createRIB(void);

///
/// Destroy a RIB
///
void destroyRIB(const rina::rib::rib_handle_t& rib);

///
/// Associate RIB to AE
///
void associateRIBtoAE(const rina::rib::rib_handle_t& rib,
						const std::string& ae_name);

///
/// Add a new IPCP object due to an external creation (e.g. CLI)
///
void createIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id);

///
/// Add a new IPCP object due to an external creation (e.g. CLI)
///
void destroyIPCPObj(const rina::rib::rib_handle_t& rib, int ipcp_id);

}  //namespace rib_v1
}  //namespace mad
}  //namespace rinad

#endif  /* __RINAD_RIBD_V1_H__ */
