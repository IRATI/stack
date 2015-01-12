#include "ribd_v1.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define RINA_PREFIX "mad.ribd_v1"
#include <librina/logs.h>

namespace rinad{
namespace mad{


void RIBDaemonv1::sendMessageSpecific(bool useAddress,
			const rina::CDAPMessage& cdapMessage,
			int sessionId,
                        unsigned int address,
			rina::ICDAPResponseMessageHandler* cdapMessageHandler){

	//XXX FIXME: fill-in
	(void)useAddress;
	(void)cdapMessage;
	(void)sessionId;
	(void)address;
	(void)cdapMessageHandler;
}


}; //namespace mad
}; //namespace rinad


