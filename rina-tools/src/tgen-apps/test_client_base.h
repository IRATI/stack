#pragma once

#include <thread>
#include <chrono>
#include <time.h>

#include "ra_commons.h"
#include "ra_base_client.h"
#include "test_commons.h"


class TestClientBase : public ra::BaseClient {
public:
	TestClientBase(const std::string Name, const std::string Instance, const std::string Servername,
		       const std::string ServerInstance, const std::string DIF,
		       int FlowIdent, int QosIdent, int TestDuration, bool verbose,
		       size_t mpdus, unsigned int loss, unsigned int delay) :
		BaseClient(Name, Instance, Servername, ServerInstance, DIF, verbose, loss, delay) {
		_FlowIdent = FlowIdent;
		_QosIdent = QosIdent;
		_TestDuration = TestDuration;
		AllocTimeoutMs = 1000;

		Data = (dataSDU*) Buffer;
		max_sdu_size = mpdus;
		_Fd = 0;
	}

	virtual ~TestClientBase(){};

protected:
	dataSDU * Data;
	std::chrono::time_point<std::chrono::system_clock> Endtime;

	virtual int HandleFlow(const int Fd) {
		_Fd = Fd;
		srand(time(0));

		initSDU * InitData = (initSDU*)Buffer;

		Data->Size = sizeof(initSDU);
		Data->Flags = SDU_FLAG_INIT;
		Data->SeqId = 0;
		Data->SendTime = 0;
		InitData->QoSId = _QosIdent;
		InitData->FlowId = _FlowIdent;

		if (SendData(sizeof(initSDU), 1000) != sizeof(initSDU)) {
			std::cerr << "No data sent during the first second of lifetime" << std::endl;
			return -1;
		}

		if (ra::ReadDataTimeout(Fd, Buffer, 1000) <= 0) {
			std::cerr << "No data echo received during the first second of lifetime" << std::endl;
			return -1;
		}

		Data->Flags = 0;
		Data->SeqId = 0;

		Endtime = std::chrono::system_clock::now() + std::chrono::seconds(_TestDuration);
		int ReturnCode = RunFlow();

		Data->Flags = SDU_FLAG_FIN;
		if (SendData(sizeof(dataSDU), 1000) != sizeof(dataSDU)) {
			std::cerr << "failure sending fin SDU" << std::endl;
			return -1;
		}

		if (ra::ReadDataTimeout(Fd, Buffer, 10000) <= 0) {
			std::cerr << "No data echo received for the fin SDU" << std::endl;
			return -1;
		}

		return ReturnCode;
	}

	int SendData(size_t Size, int mSec = -1) {
		Data->Size = Size;
		Data->SeqId++;
		Data->SendTime =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		if (mSec < 0) {
			return ra::WriteData(_Fd, (ra::byte_t*) Data, Size, max_sdu_size);
		} else {
			return ra::WriteDataTimeout(_Fd, (ra::byte_t*) Data, Size, max_sdu_size, mSec);
		}
	}

	virtual int RunFlow() { return 0;  };

private:
	int _FlowIdent, _QosIdent, _TestDuration;
	int _Fd;
	ra::byte_t Buffer[BUFF_SIZE];
	size_t max_sdu_size;
};
