#pragma once

#include <cstdint>

#ifndef BUFF_SIZE
#define BUFF_SIZE 10000
#endif

#define SDU_FLAG_INIT	1
#define SDU_FLAG_FIN	2

#pragma pack(push, 1)
struct dataSDU {
	size_t		Size;
	uint8_t		Flags;
	uint32_t	SeqId;
	long long	SendTime;
};
#pragma pack(pop)


#pragma pack(push, 1)
struct initSDU : public dataSDU {
	int QoSId;
	int FlowId;
};
#pragma pack(pop)
