//TODO

#ifndef __RINAD_RIBD_V1_H__
#define __RINAD_RIBD_V1_H__

#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include <librina/cdap.h>
#include <librina/common.h>
#include <librina/patterns.h>
#include <librina/rib.h>

#include "event-loop.h"

namespace rinad{
namespace mad{

/**
* @file ribdv1.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Management Agent RIB daemon v1
*/

//TODO FIXME: this is a place holder class
class RIBDaemonv1: public rina::RIBDaemon{
public:
	RIBDaemonv1(const rina::RIBSchema *schema);

	//XXX: fill-in
	virtual void sendMessageSpecific(bool useAddress,
			const rina::CDAPMessage& cdapMessage,
			int sessionId,
			unsigned int address,
			rina::ICDAPResponseMessageHandler* cdapMessageHandler);


};

}; //namespace mad
}; //namespace rinad

#endif  /* __RINAD_RIBD_V1_H__ */
